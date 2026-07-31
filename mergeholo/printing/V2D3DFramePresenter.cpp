#include "V2D3DFramePresenter.h"

#include <QApplication>
#include <QCoreApplication>
#include <QMetaObject>
#include <QThread>
#include <QWidget>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <limits>
#include <utility>

#ifdef Q_OS_WIN
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi1_3.h>
#include <qt_windows.h>
#include <wrl/client.h>
#endif

namespace {

constexpr int kMaximumD3D11TextureDimension = 16384;

void clearError(QString* errorMessage)
{
    if (errorMessage) errorMessage->clear();
}

void setError(QString* errorMessage, const QString& message)
{
    if (errorMessage) *errorMessage = message;
}

QString hresultText(const QString& operation, qint32 value)
{
    return QStringLiteral("%1 failed HRESULT=0x%2.")
        .arg(operation)
        .arg(static_cast<quint32>(value), 8, 16, QLatin1Char('0'));
}

class QtPresentationDispatcher final : public IPresentationDispatcher
{
public:
    bool invokeSynchronously(const std::function<bool()>& command, QString* errorMessage) override
    {
        QCoreApplication* app = QCoreApplication::instance();
        if (!app) {
            setError(errorMessage, QStringLiteral("A Qt application is required for display presentation."));
            return false;
        }
        if (QThread::currentThread() == app->thread()) return command();

        bool result = false;
        const bool invoked = QMetaObject::invokeMethod(app, [&] { result = command(); },
            Qt::BlockingQueuedConnection);
        if (!invoked) {
            setError(errorMessage, QStringLiteral("Unable to dispatch presentation command to the GUI thread."));
            return false;
        }
        return result;
    }
};

#ifdef Q_OS_WIN

using Microsoft::WRL::ComPtr;

const char* const kVertexShader =
    "struct VSOut { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };\n"
    "VSOut main(uint id : SV_VertexID) {\n"
    "float2 pos[3]={float2(-1,-1),float2(-1,3),float2(3,-1)};\n"
    "float2 uv[3]={float2(0,1),float2(0,-1),float2(2,1)};\n"
    "VSOut o; o.pos=float4(pos[id],0,1); o.uv=uv[id]; return o; }\n";

const char* const kPixelShader =
    "Texture2D tex0 : register(t0); SamplerState samp0 : register(s0);\n"
    "float4 main(float4 p:SV_POSITION,float2 uv:TEXCOORD0):SV_TARGET {"
    "return tex0.Sample(samp0,uv); }\n";

class NativeD3DBackend final : public IV2D3DBackend, public IVBlankWaiter
{
public:
    ~NativeD3DBackend() override { release(); }

    bool prepare(const DisplayMonitor& monitor, const QSize& surfaceSize,
        PresenterDiagnostics* diagnostics, QString* errorMessage) override
    {
        release();
        clearError(errorMessage);
        if (QThread::currentThread() != qApp->thread()) {
            setError(errorMessage, QStringLiteral("Presentation window must be created on the GUI thread."));
            return false;
        }
        if (!monitor.attachedToDesktop || monitor.primary || !monitor.geometry.isValid()
            || monitor.nativeMonitor == 0 || !surfaceSize.isValid()) {
            setError(errorMessage, QStringLiteral("Selected non-primary display is stale or invalid."));
            return false;
        }

        selectedMonitor_ = reinterpret_cast<HMONITOR>(monitor.nativeMonitor);
        selectedDeviceName_ = monitor.deviceName;
        window_ = new QWidget(nullptr, Qt::Window | Qt::FramelessWindowHint);
        window_->setAttribute(Qt::WA_NativeWindow);
        window_->setAttribute(Qt::WA_OpaquePaintEvent);
        window_->setAutoFillBackground(false);
        window_->setGeometry(monitor.geometry);
        window_->show();
        window_->raise();
        hwnd_ = reinterpret_cast<HWND>(window_->winId());
        if (!IsWindow(hwnd_)) {
            setError(errorMessage, QStringLiteral("Unable to create the native second-screen window."));
            release();
            return false;
        }

        HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory_));
        if (diagnostics) diagnostics->prepareHresult = hr;
        if (FAILED(hr)) return failPrepare(QStringLiteral("CreateDXGIFactory1"), hr, errorMessage);

        if (!findSelectedOutput(errorMessage)) {
            release();
            return false;
        }

        const D3D_FEATURE_LEVEL levels[] = {
            D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0
        };
        D3D_FEATURE_LEVEL selectedLevel = D3D_FEATURE_LEVEL_10_0;
        hr = D3D11CreateDevice(adapter_.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT, levels, ARRAYSIZE(levels), D3D11_SDK_VERSION,
            &device_, &selectedLevel, &context_);
        if (diagnostics) diagnostics->prepareHresult = hr;
        if (FAILED(hr)) return failPrepare(QStringLiteral("D3D11CreateDevice"), hr, errorMessage);

        ComPtr<IDXGIDevice1> dxgiDevice;
        if (SUCCEEDED(device_.As(&dxgiDevice)) && dxgiDevice) dxgiDevice->SetMaximumFrameLatency(1);

        if (!createSwapChain(monitor.geometry.size(), errorMessage)
            || !createBackBuffer(errorMessage)
            || !createShaders(errorMessage)) {
            release();
            return false;
        }
        if (!outputMatches(monitor, errorMessage)) {
            release();
            return false;
        }

        if (diagnostics) {
            diagnostics->prepareHresult = S_OK;
            diagnostics->selectedOutputDeviceName = selectedDeviceName_;
            diagnostics->vblank.outputDeviceName = selectedDeviceName_;
            diagnostics->vblank.adapterIndex = adapterIndex_;
            diagnostics->vblank.outputIndex = outputIndex_;
        }
        return true;
    }

    bool outputMatches(const DisplayMonitor& monitor, QString* errorMessage) const override
    {
        if (!window_ || !IsWindow(hwnd_) || !swapChain_ || !output_) {
            setError(errorMessage, QStringLiteral("Selected DXGI output is not prepared."));
            return false;
        }
        MONITORINFO info = {};
        info.cbSize = sizeof(info);
        if (!GetMonitorInfoW(selectedMonitor_, &info)
            || MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONULL) != selectedMonitor_
            || reinterpret_cast<quintptr>(selectedMonitor_) != monitor.nativeMonitor) {
            setError(errorMessage, QStringLiteral("Selected display disconnected or presentation window moved."));
            return false;
        }

        DXGI_OUTPUT_DESC selectedDesc = {};
        if (FAILED(output_->GetDesc(&selectedDesc)) || selectedDesc.Monitor != selectedMonitor_) {
            setError(errorMessage, QStringLiteral("Selected DXGI output no longer matches the print monitor."));
            return false;
        }
        ComPtr<IDXGIOutput> containingOutput;
        HRESULT hr = swapChain_->GetContainingOutput(&containingOutput);
        DXGI_OUTPUT_DESC containingDesc = {};
        if (FAILED(hr) || !containingOutput
            || FAILED(containingOutput->GetDesc(&containingDesc))
            || containingDesc.Monitor != selectedMonitor_) {
            setError(errorMessage, hresultText(QStringLiteral("IDXGISwapChain::GetContainingOutput"), hr)
                + QStringLiteral(" Selected output mismatch."));
            return false;
        }
        return true;
    }

    bool uploadAndPresent(const QByteArray& bgra, int width, int height, int rowPitch,
        const QSize& targetSize, PresenterDiagnostics* diagnostics, QString* errorMessage) override
    {
        clearError(errorMessage);
        if (!device_ || !context_ || !swapChain_ || !renderTargetView_) {
            setError(errorMessage, QStringLiteral("D3D presenter is not prepared."));
            return false;
        }

        D3D11_TEXTURE2D_DESC textureDesc = {};
        textureDesc.Width = static_cast<UINT>(width);
        textureDesc.Height = static_cast<UINT>(height);
        textureDesc.MipLevels = 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA data = {};
        data.pSysMem = bgra.constData();
        data.SysMemPitch = static_cast<UINT>(rowPitch);

        ComPtr<ID3D11Texture2D> texture;
        HRESULT hr = device_->CreateTexture2D(&textureDesc, &data, &texture);
        if (diagnostics) diagnostics->uploadHresult = hr;
        if (FAILED(hr)) {
            setError(errorMessage, hresultText(QStringLiteral("ID3D11Device::CreateTexture2D"), hr));
            return false;
        }
        ComPtr<ID3D11ShaderResourceView> srv;
        hr = device_->CreateShaderResourceView(texture.Get(), nullptr, &srv);
        if (diagnostics) diagnostics->uploadHresult = hr;
        if (FAILED(hr)) {
            setError(errorMessage, hresultText(QStringLiteral("ID3D11Device::CreateShaderResourceView"), hr));
            return false;
        }

        const float black[] = {0.0f, 0.0f, 0.0f, 1.0f};
        ID3D11RenderTargetView* rtv = renderTargetView_.Get();
        context_->OMSetRenderTargets(1, &rtv, nullptr);
        context_->ClearRenderTargetView(rtv, black);
        D3D11_VIEWPORT viewport = {};
        const int viewportWidth = std::min(targetSize.width(), surfaceWidth_);
        const int viewportHeight = std::min(targetSize.height(), surfaceHeight_);
        viewport.TopLeftX = static_cast<float>((surfaceWidth_ - viewportWidth) / 2);
        viewport.TopLeftY = static_cast<float>((surfaceHeight_ - viewportHeight) / 2);
        viewport.Width = static_cast<float>(viewportWidth);
        viewport.Height = static_cast<float>(viewportHeight);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        context_->RSSetViewports(1, &viewport);
        ID3D11ShaderResourceView* view = srv.Get();
        ID3D11SamplerState* sampler = sampler_.Get();
        context_->IASetInputLayout(nullptr);
        context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context_->VSSetShader(vertexShader_.Get(), nullptr, 0);
        context_->PSSetShader(pixelShader_.Get(), nullptr, 0);
        context_->PSSetShaderResources(0, 1, &view);
        context_->PSSetSamplers(0, 1, &sampler);
        context_->Draw(3, 0);

        hr = swapChain_->Present(1, 0);
        if (diagnostics) diagnostics->presentHresult = hr;
        ID3D11ShaderResourceView* noView = nullptr;
        context_->PSSetShaderResources(0, 1, &noView);
        if (FAILED(hr)) {
            setError(errorMessage, hresultText(QStringLiteral("IDXGISwapChain::Present(1,0)"), hr));
            return false;
        }
        collectTelemetry(diagnostics);
        return true;
    }

    bool waitForPhysicalVBlank(VBlankDiagnostics* diagnostics, QString* errorMessage) override
    {
        VBlankDiagnostics local;
        VBlankDiagnostics& result = diagnostics ? *diagnostics : local;
        result.outputDeviceName = selectedDeviceName_;
        result.adapterIndex = adapterIndex_;
        result.outputIndex = outputIndex_;
        if (!output_) {
            result.hresult = E_POINTER;
            setError(errorMessage, hresultText(QStringLiteral("IDXGIOutput::WaitForVBlank"), E_POINTER));
            return false;
        }
        const HRESULT hr = output_->WaitForVBlank();
        result.hresult = hr;
        if (FAILED(hr)) {
            setError(errorMessage, hresultText(QStringLiteral("IDXGIOutput::WaitForVBlank"), hr));
            return false;
        }
        clearError(errorMessage);
        return true;
    }

    void shutdown() override { release(); }

private:
    bool failPrepare(const QString& operation, HRESULT hr, QString* errorMessage)
    {
        setError(errorMessage, hresultText(operation, hr));
        release();
        return false;
    }

    bool findSelectedOutput(QString* errorMessage)
    {
        for (UINT adapterIndex = 0; ; ++adapterIndex) {
            ComPtr<IDXGIAdapter1> adapter;
            HRESULT hr = factory_->EnumAdapters1(adapterIndex, &adapter);
            if (hr == DXGI_ERROR_NOT_FOUND) break;
            if (FAILED(hr)) {
                setError(errorMessage, hresultText(QStringLiteral("IDXGIFactory1::EnumAdapters1"), hr));
                return false;
            }
            for (UINT outputIndex = 0; ; ++outputIndex) {
                ComPtr<IDXGIOutput> output;
                hr = adapter->EnumOutputs(outputIndex, &output);
                if (hr == DXGI_ERROR_NOT_FOUND) break;
                if (FAILED(hr)) {
                    setError(errorMessage, hresultText(QStringLiteral("IDXGIAdapter::EnumOutputs"), hr));
                    return false;
                }
                DXGI_OUTPUT_DESC desc = {};
                hr = output->GetDesc(&desc);
                if (SUCCEEDED(hr) && desc.Monitor == selectedMonitor_) {
                    adapter_ = adapter;
                    output_ = output;
                    adapterIndex_ = static_cast<int>(adapterIndex);
                    outputIndex_ = static_cast<int>(outputIndex);
                    selectedDeviceName_ = QString::fromWCharArray(desc.DeviceName);
                    return true;
                }
            }
        }
        setError(errorMessage, QStringLiteral("No DXGI output matches the selected non-primary monitor."));
        return false;
    }

    bool createSwapChain(const QSize& size, QString* errorMessage)
    {
        ComPtr<IDXGIFactory2> factory2;
        HRESULT hr = factory_.As(&factory2);
        if (FAILED(hr) || !factory2) {
            setError(errorMessage, hresultText(QStringLiteral("IDXGIFactory2 query"), hr));
            return false;
        }
        DXGI_SWAP_CHAIN_DESC1 desc = {};
        desc.Width = static_cast<UINT>(size.width());
        desc.Height = static_cast<UINT>(size.height());
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = 2;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        desc.Scaling = DXGI_SCALING_NONE;
        desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        desc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        ComPtr<IDXGISwapChain1> swapChain1;
        hr = factory2->CreateSwapChainForHwnd(device_.Get(), hwnd_, &desc, nullptr, output_.Get(), &swapChain1);
        if (SUCCEEDED(hr) && swapChain1) {
            swapChain1.As(&swapChain_);
            swapChain1.As(&swapChain2_);
            if (swapChain2_ && SUCCEEDED(swapChain2_->SetMaximumFrameLatency(1))) {
                frameLatencyHandle_ = swapChain2_->GetFrameLatencyWaitableObject();
            }
        }
        if (!swapChain_) {
            DXGI_SWAP_CHAIN_DESC legacy = {};
            legacy.BufferDesc.Width = static_cast<UINT>(size.width());
            legacy.BufferDesc.Height = static_cast<UINT>(size.height());
            legacy.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            legacy.BufferDesc.RefreshRate.Denominator = 1;
            legacy.SampleDesc.Count = 1;
            legacy.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            legacy.BufferCount = 2;
            legacy.OutputWindow = hwnd_;
            legacy.Windowed = TRUE;
            legacy.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
            hr = factory_->CreateSwapChain(device_.Get(), &legacy, &swapChain_);
        }
        if (FAILED(hr) || !swapChain_) {
            setError(errorMessage, hresultText(QStringLiteral("DXGI swap-chain creation"), hr));
            return false;
        }
        factory_->MakeWindowAssociation(hwnd_, DXGI_MWA_NO_ALT_ENTER);
        surfaceWidth_ = size.width();
        surfaceHeight_ = size.height();
        return true;
    }

    bool createBackBuffer(QString* errorMessage)
    {
        ComPtr<ID3D11Texture2D> backBuffer;
        HRESULT hr = swapChain_->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
        if (SUCCEEDED(hr)) hr = device_->CreateRenderTargetView(backBuffer.Get(), nullptr, &renderTargetView_);
        if (FAILED(hr)) setError(errorMessage, hresultText(QStringLiteral("D3D back-buffer creation"), hr));
        return SUCCEEDED(hr);
    }

    bool createShaders(QString* errorMessage)
    {
        ComPtr<ID3DBlob> vertexBlob;
        ComPtr<ID3DBlob> pixelBlob;
        ComPtr<ID3DBlob> errors;
        HRESULT hr = D3DCompile(kVertexShader, std::strlen(kVertexShader), nullptr, nullptr, nullptr,
            "main", "vs_4_0", 0, 0, &vertexBlob, &errors);
        if (FAILED(hr)) {
            setError(errorMessage, hresultText(QStringLiteral("D3DCompile vertex shader"), hr));
            return false;
        }
        hr = D3DCompile(kPixelShader, std::strlen(kPixelShader), nullptr, nullptr, nullptr,
            "main", "ps_4_0", 0, 0, &pixelBlob, &errors);
        if (FAILED(hr)) {
            setError(errorMessage, hresultText(QStringLiteral("D3DCompile pixel shader"), hr));
            return false;
        }
        hr = device_->CreateVertexShader(vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(),
            nullptr, &vertexShader_);
        if (SUCCEEDED(hr)) {
            hr = device_->CreatePixelShader(pixelBlob->GetBufferPointer(), pixelBlob->GetBufferSize(),
                nullptr, &pixelShader_);
        }
        if (FAILED(hr)) {
            setError(errorMessage, hresultText(QStringLiteral("D3D shader creation"), hr));
            return false;
        }
        D3D11_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
        hr = device_->CreateSamplerState(&samplerDesc, &sampler_);
        if (FAILED(hr)) setError(errorMessage, hresultText(QStringLiteral("D3D sampler creation"), hr));
        return SUCCEEDED(hr);
    }

    void collectTelemetry(PresenterDiagnostics* diagnostics)
    {
        if (!diagnostics) return;
        DXGI_FRAME_STATISTICS statistics = {};
        HRESULT hr = swapChain_->GetFrameStatistics(&statistics);
        diagnostics->frameStatisticsHresult = hr;
        if (SUCCEEDED(hr)) {
            diagnostics->presentCount = statistics.PresentCount;
            diagnostics->presentRefreshCount = statistics.PresentRefreshCount;
            diagnostics->syncRefreshCount = statistics.SyncRefreshCount;
        }
        ComPtr<IDXGISwapChainMedia> media;
        hr = swapChain_.As(&media);
        if (SUCCEEDED(hr) && media) {
            DXGI_FRAME_STATISTICS_MEDIA mediaStatistics = {};
            hr = media->GetFrameStatisticsMedia(&mediaStatistics);
        }
        diagnostics->frameStatisticsMediaHresult = hr;
        diagnostics->frameLatencyWaitResult = frameLatencyHandle_
            ? static_cast<qint64>(WaitForSingleObjectEx(frameLatencyHandle_, 0, FALSE)) : -1;
    }

    void release()
    {
        if (context_) {
            ID3D11RenderTargetView* noTarget = nullptr;
            context_->OMSetRenderTargets(1, &noTarget, nullptr);
            context_->ClearState();
            context_->Flush();
        }
        sampler_.Reset();
        pixelShader_.Reset();
        vertexShader_.Reset();
        renderTargetView_.Reset();
        frameLatencyHandle_ = nullptr;
        swapChain2_.Reset();
        swapChain_.Reset();
        context_.Reset();
        device_.Reset();
        output_.Reset();
        adapter_.Reset();
        factory_.Reset();
        if (window_) {
            window_->hide();
            delete window_;
            window_ = nullptr;
        }
        hwnd_ = nullptr;
        selectedMonitor_ = nullptr;
        selectedDeviceName_.clear();
        adapterIndex_ = -1;
        outputIndex_ = -1;
        surfaceWidth_ = 0;
        surfaceHeight_ = 0;
    }

    QWidget* window_ = nullptr;
    HWND hwnd_ = nullptr;
    HMONITOR selectedMonitor_ = nullptr;
    QString selectedDeviceName_;
    int adapterIndex_ = -1;
    int outputIndex_ = -1;
    int surfaceWidth_ = 0;
    int surfaceHeight_ = 0;
    HANDLE frameLatencyHandle_ = nullptr;
    ComPtr<IDXGIFactory1> factory_;
    ComPtr<IDXGIAdapter1> adapter_;
    ComPtr<IDXGIOutput> output_;
    ComPtr<ID3D11Device> device_;
    ComPtr<ID3D11DeviceContext> context_;
    ComPtr<IDXGISwapChain> swapChain_;
    ComPtr<IDXGISwapChain2> swapChain2_;
    ComPtr<ID3D11RenderTargetView> renderTargetView_;
    ComPtr<ID3D11VertexShader> vertexShader_;
    ComPtr<ID3D11PixelShader> pixelShader_;
    ComPtr<ID3D11SamplerState> sampler_;
};

#else

class NativeD3DBackend final : public IV2D3DBackend, public IVBlankWaiter
{
public:
    bool prepare(const DisplayMonitor&, const QSize&, PresenterDiagnostics*, QString* errorMessage) override
    { setError(errorMessage, QStringLiteral("V2 D3D presentation is Windows-only.")); return false; }
    bool outputMatches(const DisplayMonitor&, QString* errorMessage) const override
    { setError(errorMessage, QStringLiteral("V2 D3D presentation is Windows-only.")); return false; }
    bool uploadAndPresent(const QByteArray&, int, int, int, const QSize&, PresenterDiagnostics*, QString* errorMessage) override
    { setError(errorMessage, QStringLiteral("V2 D3D presentation is Windows-only.")); return false; }
    bool waitForPhysicalVBlank(VBlankDiagnostics*, QString* errorMessage) override
    { setError(errorMessage, QStringLiteral("Physical DXGI VBlank is Windows-only.")); return false; }
    void shutdown() override {}
};

#endif

} // namespace

V2D3DFramePresenter::V2D3DFramePresenter()
    : dispatcher_(std::make_shared<QtPresentationDispatcher>())
{
    const auto native = std::make_shared<NativeD3DBackend>();
    backend_ = native;
    vblankWaiter_ = native;
}

bool V2D3DFramePresenter::refreshAttachedDisplays(QString* errorMessage)
{
    QString detail;
    QVector<DisplayMonitor> displays = enumerateAttachedDesktopMonitors(&detail);
    if (!detail.isEmpty()) {
        setError(errorMessage, detail);
        return false;
    }
    displays_ = std::move(displays);
    if (errorMessage) errorMessage->clear();
    return true;
}

V2D3DFramePresenter::V2D3DFramePresenter(QVector<DisplayMonitor> displays,
    std::shared_ptr<IV2D3DBackend> backend, std::shared_ptr<IVBlankWaiter> vblankWaiter,
    std::shared_ptr<IPresentationDispatcher> dispatcher)
    : displays_(std::move(displays))
    , backend_(std::move(backend))
    , vblankWaiter_(std::move(vblankWaiter))
    , dispatcher_(std::move(dispatcher))
{
}

V2D3DFramePresenter::~V2D3DFramePresenter()
{
    shutdown();
}

bool V2D3DFramePresenter::convertFrame(const PrintFrame& frame, const QSize& targetSize,
    QByteArray* packedBgra, QString* errorMessage) const
{
    clearError(errorMessage);
    if (!packedBgra) {
        setError(errorMessage, QStringLiteral("Converted frame destination is unavailable."));
        return false;
    }
    packedBgra->clear();
    if (frame.pixels.isNull() || frame.pixels.isEmpty()) {
        setError(errorMessage, QStringLiteral("Print frame bytes are null or empty."));
        return false;
    }
    if (frame.width <= 0 || frame.height <= 0 || frame.width > kMaximumD3D11TextureDimension
        || frame.height > kMaximumD3D11TextureDimension) {
        setError(errorMessage, QStringLiteral("Print frame dimensions are invalid or exceed D3D11 limits."));
        return false;
    }
    if (targetSize.width() <= 0 || targetSize.height() <= 0
        || targetSize.width() > kMaximumD3D11TextureDimension
        || targetSize.height() > kMaximumD3D11TextureDimension) {
        setError(errorMessage, QStringLiteral("Presentation target size is invalid or exceeds D3D11 limits."));
        return false;
    }

    int sourceBytesPerPixel = 0;
    switch (frame.format) {
    case PrintPixelFormat::Bgr24: sourceBytesPerPixel = 3; break;
    case PrintPixelFormat::Bgra32: sourceBytesPerPixel = 4; break;
    default:
        setError(errorMessage, QStringLiteral("Unsupported print frame pixel format."));
        return false;
    }
    const quint64 sourceRowBytes = static_cast<quint64>(frame.width)
        * static_cast<quint64>(sourceBytesPerPixel);
    const quint64 outputRowBytes = static_cast<quint64>(frame.width) * 4u;
    if (sourceRowBytes > static_cast<quint64>(std::numeric_limits<int>::max())
        || outputRowBytes > static_cast<quint64>(std::numeric_limits<int>::max())
        || frame.stride <= 0 || static_cast<quint64>(frame.stride) < sourceRowBytes) {
        setError(errorMessage, QStringLiteral("Print frame stride is invalid or undersized."));
        return false;
    }
    const quint64 requiredSourceBytes = static_cast<quint64>(frame.height - 1)
        * static_cast<quint64>(frame.stride) + sourceRowBytes;
    const quint64 requiredOutputBytes = static_cast<quint64>(frame.height) * outputRowBytes;
    if (requiredSourceBytes > static_cast<quint64>(frame.pixels.size())) {
        setError(errorMessage, QStringLiteral("Print frame byte buffer is truncated for its stride and dimensions."));
        return false;
    }
    if (requiredOutputBytes == 0
        || requiredOutputBytes > static_cast<quint64>(std::numeric_limits<int>::max())) {
        setError(errorMessage, QStringLiteral("Converted BGRA frame size overflows the supported allocation."));
        return false;
    }

    packedBgra->resize(static_cast<int>(requiredOutputBytes));
    const auto* source = reinterpret_cast<const unsigned char*>(frame.pixels.constData());
    auto* destination = reinterpret_cast<unsigned char*>(packedBgra->data());
    for (int y = 0; y < frame.height; ++y) {
        const unsigned char* sourceRow = source + static_cast<size_t>(y) * static_cast<size_t>(frame.stride);
        unsigned char* destinationRow = destination
            + static_cast<size_t>(y) * static_cast<size_t>(outputRowBytes);
        for (int x = 0; x < frame.width; ++x) {
            const unsigned char* sourcePixel = sourceRow + x * sourceBytesPerPixel;
            unsigned char* destinationPixel = destinationRow + x * 4;
            destinationPixel[0] = sourcePixel[0];
            destinationPixel[1] = sourcePixel[1];
            destinationPixel[2] = sourcePixel[2];
            destinationPixel[3] = sourceBytesPerPixel == 4 ? sourcePixel[3] : 255;
        }
    }
    return true;
}

bool V2D3DFramePresenter::prepare(const PrintFrame& firstFrame, const QSize& targetSize,
    QString* errorMessage)
{
    QByteArray bgra;
    if (!convertFrame(firstFrame, targetSize, &bgra, errorMessage)) return false;
    if (!backend_ || !vblankWaiter_ || !dispatcher_) {
        setError(errorMessage, QStringLiteral("V2 presenter dependencies are unavailable."));
        invalidateReadyState();
        return false;
    }
    const std::optional<int> selected = selectV2SecondScreenIndex(displays_);
    if (!selected) {
        shutdown();
        setError(errorMessage, QStringLiteral("A valid attached non-primary display is required for printing."));
        return false;
    }
    const DisplayMonitor selectedMonitor = displays_.at(*selected);
    quint64 commandGeneration = 0;
    {
        QMutexLocker stateLock(&stateMutex_);
        commandGeneration = ++generation_;
        ready_ = false;
        diagnostics_ = {};
        diagnostics_.selectedOutputDeviceName = selectedMonitor.deviceName;
        selectedMonitor_ = selectedMonitor;
    }

    std::atomic_bool commandExecuted{false};
    const bool ok = dispatcher_->invokeSynchronously([&, commandGeneration, selectedMonitor] {
        commandExecuted.store(true);
        bool oldBackendActive = false;
        {
            QMutexLocker stateLock(&stateMutex_);
            if (generation_ != commandGeneration) return false;
            oldBackendActive = backendActive_;
            backendActive_ = false;
        }
        if (oldBackendActive) backend_->shutdown();
        {
            QMutexLocker stateLock(&stateMutex_);
            if (generation_ != commandGeneration) return false;
        }

        PresenterDiagnostics localDiagnostics;
        localDiagnostics.selectedOutputDeviceName = selectedMonitor.deviceName;
        const bool prepared = backend_->prepare(selectedMonitor, selectedMonitor.geometry.size(),
            &localDiagnostics, errorMessage)
            && backend_->outputMatches(selectedMonitor, errorMessage)
            && backend_->uploadAndPresent(bgra, firstFrame.width, firstFrame.height,
                firstFrame.width * 4, targetSize, &localDiagnostics, errorMessage);
        if (prepared) ++localDiagnostics.orderedPresentCount;

        bool stale = false;
        {
            QMutexLocker stateLock(&stateMutex_);
            stale = generation_ != commandGeneration;
            if (!stale) {
                diagnostics_ = localDiagnostics;
                ready_ = prepared;
                diagnostics_.ready = prepared;
                backendActive_ = prepared;
                if (!prepared) ++generation_;
            }
        }
        if (!prepared || stale) backend_->shutdown();
        return prepared && !stale;
    }, errorMessage);
    if (!ok && !commandExecuted.load()) {
        failDispatcherCommand(commandGeneration, QStringLiteral("Prepare"), errorMessage);
    }
    return ok;
}

PrintPresenterReadiness V2D3DFramePresenter::printReadiness(QString* errorMessage) const
{
    PrintPresenterReadiness readiness;
    readiness.secondScreenAttached = selectV2SecondScreenIndex(displays_).has_value();
    readiness.presenterAvailable = backend_ && vblankWaiter_ && dispatcher_;
    readiness.vblankReady = readiness.presenterAvailable;
    readiness.generationCurrent = readiness.presenterAvailable;
    if (errorMessage) {
        errorMessage->clear();
        if (!readiness.secondScreenAttached)
            *errorMessage = QStringLiteral("No attached non-primary display is available.");
        else if (!readiness.presenterAvailable)
            *errorMessage = QStringLiteral("Presenter backend, dispatcher, or VBlank waiter is unavailable.");
    }
    return readiness;
}

double V2D3DFramePresenter::selectedRefreshHz(QString* errorMessage) const
{
    if (errorMessage) errorMessage->clear();
    const std::optional<int> selected = selectV2SecondScreenIndex(displays_);
    if (!selected) {
        setError(errorMessage,
            QStringLiteral("A valid attached non-primary display is required to determine refresh rate."));
        return 0.0;
    }
    const double refreshHz = displays_.at(*selected).refreshHz;
    if (!std::isfinite(refreshHz) || refreshHz <= 1.0) {
        setError(errorMessage,
            QStringLiteral("The selected print display refresh rate is unavailable or invalid."));
        return 0.0;
    }
    return refreshHz;
}

bool V2D3DFramePresenter::present(const PrintFrame& frame, const QSize& targetSize,
    QString* errorMessage)
{
    QByteArray bgra;
    if (!convertFrame(frame, targetSize, &bgra, errorMessage)) {
        invalidateReadyState();
        return false;
    }
    quint64 expectedGeneration = 0;
    DisplayMonitor selectedMonitor;
    PresenterDiagnostics localDiagnostics;
    {
        QMutexLocker stateLock(&stateMutex_);
        if (!ready_) {
            setError(errorMessage, QStringLiteral("V2 presenter is not ready."));
            return false;
        }
        expectedGeneration = generation_;
        selectedMonitor = selectedMonitor_;
        localDiagnostics = diagnostics_;
    }
    std::atomic_bool commandExecuted{false};
    const bool presented = dispatcher_->invokeSynchronously([&, expectedGeneration, selectedMonitor] {
        commandExecuted.store(true);
        {
            QMutexLocker stateLock(&stateMutex_);
            if (!ready_ || generation_ != expectedGeneration) {
                setError(errorMessage, QStringLiteral("Presentation command was invalidated before execution."));
                return false;
            }
        }
        const bool presented = backend_->outputMatches(selectedMonitor, errorMessage)
            && backend_->uploadAndPresent(bgra, frame.width, frame.height, frame.width * 4,
                targetSize, &localDiagnostics, errorMessage);
        if (presented) ++localDiagnostics.orderedPresentCount;
        QMutexLocker stateLock(&stateMutex_);
        if (generation_ != expectedGeneration) return false;
        diagnostics_ = localDiagnostics;
        if (!presented) {
            ready_ = false;
            diagnostics_.ready = false;
            ++generation_;
        }
        return presented;
    }, errorMessage);
    if (!presented && !commandExecuted.load()) {
        failDispatcherCommand(expectedGeneration, QStringLiteral("Present"), errorMessage);
    }
    return presented;
}

bool V2D3DFramePresenter::presentRowAnchor(const PrintFrame& frame,
    const QSize& targetSize, QString* errorMessage)
{
    return present(frame, targetSize, errorMessage);
}

bool V2D3DFramePresenter::waitForDisplayFrame(QString* errorMessage)
{
    quint64 expectedGeneration = 0;
    DisplayMonitor selectedMonitor;
    PresenterDiagnostics localDiagnostics;
    {
        QMutexLocker stateLock(&stateMutex_);
        if (!ready_) {
            setError(errorMessage, QStringLiteral("V2 presenter is not ready for physical VBlank."));
            return false;
        }
        expectedGeneration = generation_;
        selectedMonitor = selectedMonitor_;
        localDiagnostics = diagnostics_;
    }
    std::atomic_bool commandExecuted{false};
    const bool waited = dispatcher_->invokeSynchronously([&, expectedGeneration, selectedMonitor] {
        commandExecuted.store(true);
        {
            QMutexLocker stateLock(&stateMutex_);
            if (!ready_ || generation_ != expectedGeneration) {
                setError(errorMessage, QStringLiteral("VBlank command was invalidated before execution."));
                return false;
            }
        }
        const bool waited = backend_->outputMatches(selectedMonitor, errorMessage)
            && vblankWaiter_->waitForPhysicalVBlank(&localDiagnostics.vblank, errorMessage);
        QMutexLocker stateLock(&stateMutex_);
        if (generation_ != expectedGeneration) return false;
        diagnostics_ = localDiagnostics;
        if (!waited) {
            ready_ = false;
            diagnostics_.ready = false;
            ++generation_;
        }
        return waited;
    }, errorMessage);
    if (!waited && !commandExecuted.load()) {
        failDispatcherCommand(expectedGeneration, QStringLiteral("physical VBlank"), errorMessage);
    }
    return waited;
}

bool V2D3DFramePresenter::acquireRowAnchor(V2RowVBlankAnchor* anchor, QString* errorMessage)
{
    if (!anchor) {
        setError(errorMessage, QStringLiteral("Row VBlank anchor destination is unavailable."));
        return false;
    }
    anchor->available = false;
    anchor->presenterGeneration = 0;
    if (!waitForDisplayFrame(errorMessage)) return false;
    QMutexLocker stateLock(&stateMutex_);
    if (!ready_) {
        setError(errorMessage, QStringLiteral("Row VBlank anchor was invalidated after acquisition."));
        return false;
    }
    anchor->presenterGeneration = generation_;
    anchor->available = ready_;
    return true;
}

bool V2D3DFramePresenter::waitForRowSlot(int slot, V2RowVBlankAnchor* anchor,
    QString* errorMessage)
{
    if (slot < 0 || !anchor) {
        setError(errorMessage, QStringLiteral("Row display slot or VBlank anchor is invalid."));
        return false;
    }
    if (slot == 0) {
        QMutexLocker stateLock(&stateMutex_);
        if (!ready_ || !anchor->available || anchor->presenterGeneration != generation_) {
            setError(errorMessage, QStringLiteral("The first row slot requires an unconsumed physical VBlank anchor."));
            return false;
        }
        anchor->available = false;
        clearError(errorMessage);
        return true;
    }
    return waitForDisplayFrame(errorMessage);
}

void V2D3DFramePresenter::invalidateReadyState()
{
    QMutexLocker stateLock(&stateMutex_);
    ready_ = false;
    diagnostics_.ready = false;
    ++generation_;
}

void V2D3DFramePresenter::failDispatcherCommand(quint64 expectedGeneration,
    const QString& commandName, QString* errorMessage)
{
    const QString message = QStringLiteral("Presentation dispatcher rejected the %1 command.")
                                .arg(commandName);
    setError(errorMessage, message);
    QMutexLocker stateLock(&stateMutex_);
    if (generation_ != expectedGeneration) return;
    ready_ = false;
    diagnostics_.ready = false;
    diagnostics_.dispatcherFailure = message;
    ++generation_;
}

void V2D3DFramePresenter::shutdown()
{
    invalidateReadyState();
    if (!backend_ || !dispatcher_) return;
    {
        QMutexLocker stateLock(&stateMutex_);
        if (!backendActive_) return;
    }
    std::atomic_bool commandExecuted{false};
    const bool dispatched = dispatcher_->invokeSynchronously([&] {
        commandExecuted.store(true);
        bool shouldShutdown = false;
        {
            QMutexLocker stateLock(&stateMutex_);
            shouldShutdown = backendActive_;
            backendActive_ = false;
        }
        if (shouldShutdown) backend_->shutdown();
        return true;
    }, nullptr);
    if (!dispatched && !commandExecuted.load()) {
        QMutexLocker stateLock(&stateMutex_);
        diagnostics_.dispatcherFailure =
            QStringLiteral("Presentation dispatcher rejected the Shutdown command.");
    }
}

bool V2D3DFramePresenter::isReady() const
{
    QMutexLocker lock(&stateMutex_);
    return ready_;
}

PresenterDiagnostics V2D3DFramePresenter::diagnostics() const
{
    QMutexLocker lock(&stateMutex_);
    return diagnostics_;
}
