#include "LegacyD3DImageRenderer.h"

#include <d3dcompiler.h>

#include <cstring>

using Microsoft::WRL::ComPtr;

namespace {

const char* kVertexShaderSource =
    "struct VSOut { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };\n"
    "VSOut main(uint id : SV_VertexID)\n"
    "{\n"
    "    float2 pos[3] = { float2(-1.0, -1.0), float2(-1.0, 3.0), float2(3.0, -1.0) };\n"
    "    float2 uv[3] = { float2(0.0, 1.0), float2(0.0, -1.0), float2(2.0, 1.0) };\n"
    "    VSOut o;\n"
    "    o.pos = float4(pos[id], 0.0, 1.0);\n"
    "    o.uv = uv[id];\n"
    "    return o;\n"
    "}\n";

const char* kPixelShaderSource =
    "Texture2D tex0 : register(t0);\n"
    "SamplerState samp0 : register(s0);\n"
    "float4 main(float4 pos : SV_POSITION, float2 uv : TEXCOORD0) : SV_TARGET\n"
    "{\n"
    "    return tex0.Sample(samp0, uv);\n"
    "}\n";

bool compileShader(const char* source, const char* entry, const char* target, ID3DBlob** blob)
{
    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3DCOMPILE_DEBUG;
#endif
    ComPtr<ID3DBlob> errors;
    return SUCCEEDED(D3DCompile(
        source,
        std::strlen(source),
        nullptr,
        nullptr,
        nullptr,
        entry,
        target,
        flags,
        0,
        blob,
        &errors));
}

} // namespace

LegacyD3DImageRenderer::LegacyD3DImageRenderer() = default;

LegacyD3DImageRenderer::~LegacyD3DImageRenderer()
{
    releaseAll();
}

bool LegacyD3DImageRenderer::initialize(HWND hwnd, int width, int height)
{
    releaseAll();
    if (!::IsWindow(hwnd)) {
        return false;
    }
    if (width <= 0 || height <= 0) {
        return false;
    }

    hwnd_ = hwnd;
    if (!createDeviceAndSwapChain(hwnd, width, height) || !createBackBuffer() || !createShaders()) {
        releaseAll();
        return false;
    }
    width_ = width;
    height_ = height;
    return true;
}

bool LegacyD3DImageRenderer::resize(int width, int height)
{
    if (width <= 0 || height <= 0 || !swapChain_) {
        return false;
    }
    if (width == width_ && height == height_ && renderTargetView_) {
        return true;
    }

    releaseBackBuffer();
    if (FAILED(swapChain_->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0))) {
        return false;
    }
    width_ = width;
    height_ = height;
    return createBackBuffer();
}

bool LegacyD3DImageRenderer::renderFrame(const PrintFrame& frame)
{
    if (!isReady() || !frame.isValid() || !uploadFrame(frame)) {
        return false;
    }

    const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    context_->OMSetRenderTargets(1, renderTargetView_.GetAddressOf(), nullptr);
    context_->ClearRenderTargetView(renderTargetView_.Get(), clearColor);

    D3D11_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(width_);
    viewport.Height = static_cast<float>(height_);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context_->RSSetViewports(1, &viewport);

    ID3D11ShaderResourceView* srv = imageSrv_.Get();
    ID3D11SamplerState* sampler = samplerState_.Get();
    context_->IASetInputLayout(nullptr);
    context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context_->VSSetShader(vertexShader_.Get(), nullptr, 0);
    context_->PSSetShader(pixelShader_.Get(), nullptr, 0);
    context_->PSSetShaderResources(0, 1, &srv);
    context_->PSSetSamplers(0, 1, &sampler);
    context_->Draw(3, 0);

    const HRESULT result = swapChain_->Present(0, 0);
    ID3D11ShaderResourceView* nullSrv = nullptr;
    context_->PSSetShaderResources(0, 1, &nullSrv);
    return SUCCEEDED(result);
}

bool LegacyD3DImageRenderer::waitForVBlank()
{
    if (!swapChain_) {
        return false;
    }
    ComPtr<IDXGIOutput> output;
    return SUCCEEDED(swapChain_->GetContainingOutput(&output))
        && output
        && SUCCEEDED(output->WaitForVBlank());
}

bool LegacyD3DImageRenderer::isReady() const
{
    return device_ && context_ && swapChain_ && renderTargetView_ && vertexShader_ && pixelShader_ && samplerState_;
}

bool LegacyD3DImageRenderer::createDeviceAndSwapChain(HWND hwnd, int width, int height)
{
    DXGI_SWAP_CHAIN_DESC swapDescription = {};
    swapDescription.BufferDesc.Width = static_cast<UINT>(width);
    swapDescription.BufferDesc.Height = static_cast<UINT>(height);
    swapDescription.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapDescription.BufferDesc.RefreshRate.Denominator = 1;
    swapDescription.SampleDesc.Count = 1;
    swapDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapDescription.BufferCount = 2;
    swapDescription.OutputWindow = hwnd;
    swapDescription.Windowed = TRUE;
    swapDescription.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    const D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };
    D3D_FEATURE_LEVEL level = D3D_FEATURE_LEVEL_11_0;
    const UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

    HRESULT result = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        flags,
        levels,
        ARRAYSIZE(levels),
        D3D11_SDK_VERSION,
        &swapDescription,
        &swapChain_,
        &device_,
        &level,
        &context_);
    if (FAILED(result)) {
        result = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            flags,
            levels,
            ARRAYSIZE(levels),
            D3D11_SDK_VERSION,
            &swapDescription,
            &swapChain_,
            &device_,
            &level,
            &context_);
    }
    return SUCCEEDED(result);
}

bool LegacyD3DImageRenderer::createBackBuffer()
{
    ComPtr<ID3D11Texture2D> backBuffer;
    if (!device_ || !swapChain_
        || FAILED(swapChain_->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer.GetAddressOf())))) {
        return false;
    }
    return SUCCEEDED(device_->CreateRenderTargetView(backBuffer.Get(), nullptr, &renderTargetView_));
}

bool LegacyD3DImageRenderer::createShaders()
{
    ComPtr<ID3DBlob> vertexBlob;
    ComPtr<ID3DBlob> pixelBlob;
    if (!compileShader(kVertexShaderSource, "main", "vs_4_0", &vertexBlob)
        || !compileShader(kPixelShaderSource, "main", "ps_4_0", &pixelBlob)
        || FAILED(device_->CreateVertexShader(
            vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(), nullptr, &vertexShader_))
        || FAILED(device_->CreatePixelShader(
            pixelBlob->GetBufferPointer(), pixelBlob->GetBufferSize(), nullptr, &pixelShader_))) {
        return false;
    }

    D3D11_SAMPLER_DESC samplerDescription = {};
    samplerDescription.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDescription.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDescription.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDescription.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDescription.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDescription.MaxLOD = D3D11_FLOAT32_MAX;
    return SUCCEEDED(device_->CreateSamplerState(&samplerDescription, &samplerState_));
}

bool LegacyD3DImageRenderer::ensureImageTexture(int width, int height)
{
    if (width <= 0 || height <= 0 || !device_) {
        return false;
    }
    if (imageTexture_ && imageSrv_ && width == imageWidth_ && height == imageHeight_) {
        return true;
    }

    imageTexture_.Reset();
    imageSrv_.Reset();
    imageWidth_ = width;
    imageHeight_ = height;

    D3D11_TEXTURE2D_DESC textureDescription = {};
    textureDescription.Width = static_cast<UINT>(width);
    textureDescription.Height = static_cast<UINT>(height);
    textureDescription.MipLevels = 1;
    textureDescription.ArraySize = 1;
    textureDescription.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    textureDescription.SampleDesc.Count = 1;
    textureDescription.Usage = D3D11_USAGE_DEFAULT;
    textureDescription.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    if (FAILED(device_->CreateTexture2D(&textureDescription, nullptr, &imageTexture_))) {
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDescription = {};
    srvDescription.Format = textureDescription.Format;
    srvDescription.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDescription.Texture2D.MipLevels = 1;
    return SUCCEEDED(device_->CreateShaderResourceView(imageTexture_.Get(), &srvDescription, &imageSrv_));
}

bool LegacyD3DImageRenderer::uploadFrame(const PrintFrame& frame)
{
    if (!ensureImageTexture(frame.width, frame.height)) {
        return false;
    }

    const size_t pixelCount = static_cast<size_t>(frame.width) * static_cast<size_t>(frame.height);
    const size_t uploadSize = pixelCount * 4U;
    pixels_.resize(uploadSize);
    const unsigned char* source = reinterpret_cast<const unsigned char*>(frame.pixels.constData());
    if (frame.format == PrintPixelFormat::Bgra32) {
        std::memcpy(pixels_.data(), source, uploadSize);
    } else {
        for (size_t index = 0; index < pixelCount; ++index) {
            pixels_[index * 4U] = source[index * 3U];
            pixels_[index * 4U + 1U] = source[index * 3U + 1U];
            pixels_[index * 4U + 2U] = source[index * 3U + 2U];
            pixels_[index * 4U + 3U] = 255;
        }
    }

    context_->UpdateSubresource(imageTexture_.Get(), 0, nullptr, pixels_.data(), frame.width * 4, 0);
    return true;
}

void LegacyD3DImageRenderer::releaseBackBuffer()
{
    if (context_) {
        ID3D11RenderTargetView* nullRenderTarget = nullptr;
        context_->OMSetRenderTargets(1, &nullRenderTarget, nullptr);
    }
    renderTargetView_.Reset();
}

void LegacyD3DImageRenderer::releaseAll()
{
    releaseBackBuffer();
    imageSrv_.Reset();
    imageTexture_.Reset();
    samplerState_.Reset();
    pixelShader_.Reset();
    vertexShader_.Reset();
    swapChain_.Reset();
    context_.Reset();
    device_.Reset();
    pixels_.clear();
    hwnd_ = nullptr;
    width_ = 0;
    height_ = 0;
    imageWidth_ = 0;
    imageHeight_ = 0;
}
