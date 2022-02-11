#pragma once

#include <memory>
#include <vector>
#include <d3d11_1.h>

class Graphics {
public:
    // factory method
    static std::shared_ptr<Graphics> init(HWND hWnd);

    // render the frame
    void render();

    // cleanup all d3d objects
    void cleanup();

private:
    // forbid constructors for fabric pattern and DirectX reasons
    Graphics() = default;
    Graphics& operator=(Graphics const&) = delete;
    Graphics(Graphics const&) = delete;

    ID3D11Device* device = nullptr;
    ID3D11Device1* device1 = nullptr;
    ID3D11DeviceContext* context = nullptr;
    ID3D11DeviceContext1* context1 = nullptr;
    IDXGISwapChain* swapChain = nullptr;
    IDXGISwapChain1* swapChain1 = nullptr;
    ID3D11RenderTargetView* renderTargetView = nullptr;
};
