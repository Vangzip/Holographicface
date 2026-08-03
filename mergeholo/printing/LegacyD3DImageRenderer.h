#pragma once

#include "PrintFrame.h"

#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

#include <vector>

class LegacyD3DImageRenderer
{
public:
    LegacyD3DImageRenderer();
    ~LegacyD3DImageRenderer();

    bool initialize(HWND hwnd, int width, int height);
    bool resize(int width, int height);
    bool renderFrame(const PrintFrame& frame);
    bool waitForVBlank();
    bool isReady() const;

private:
    bool createDeviceAndSwapChain(HWND hwnd, int width, int height);
    bool createBackBuffer();
    bool createShaders();
    bool ensureImageTexture(int width, int height);
    bool uploadFrame(const PrintFrame& frame);
    void releaseBackBuffer();
    void releaseAll();

    HWND hwnd_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    int imageWidth_ = 0;
    int imageHeight_ = 0;

    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain_;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView_;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader_;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState_;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> imageTexture_;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> imageSrv_;
    std::vector<unsigned char> pixels_;
};
