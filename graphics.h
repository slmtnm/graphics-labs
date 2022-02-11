#pragma once

#include <memory>
#include <vector>
#include <d3d11_1.h>
#include <directxmath.h>


class Graphics {
public:
    // factory method
    static std::shared_ptr<Graphics> init(HWND hWnd);

    void initGeometry();

    // compile shader
    static HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);

    // render the frame
    void render();

    // cleanup all d3d objects
    void cleanup();

    // helper for releasing d3d object with checking number of residual references
    void releaseWithCheck(IUnknown *object);

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

    //------------//
    ID3D11VertexShader* vertexShader = nullptr;
    ID3D11PixelShader* pixelShader = nullptr;
    ID3D11InputLayout* vertexLayout = nullptr;
    ID3D11Buffer* vertexBuffer = nullptr;
    ID3D11Buffer* indexBuffer = nullptr;


    using SimpleVertex = DirectX::XMFLOAT3;
};