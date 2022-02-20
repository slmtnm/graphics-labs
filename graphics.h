#pragma once

#include <memory>
#include <vector>
#include <chrono>
#include <d3d11_1.h>
#include <directxmath.h>

#include "camera.h"

using namespace DirectX;

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

    // resize swapchain buffers
    HRESULT resizeBackbuffer(UINT width, UINT height);

    void setMoveLeft(bool move);
    void setMoveRight(bool move);
    void setMoveForward(bool move);
    void setMoveBackward(bool move);
    void setMoveUp(bool move);
    void setMoveDown(bool move);

    void rotate(int mouseDeltaX, int mouseDeltaY);

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
    ID3D11Buffer* constBuffer = nullptr;

    ID3DUserDefinedAnnotation* annotation = nullptr;

    struct SimpleVertex
    {
        XMFLOAT3 Pos;
        XMFLOAT4 Color;
    };

    struct ConstantBuffer
    {
        XMMATRIX mWorld;
        XMMATRIX mView;
        XMMATRIX mProjection;
        XMMATRIX mTranslation;
    };

    XMMATRIX world;
    Camera camera;

    std::chrono::system_clock::time_point start;

    // movement flags
    bool moveRight = false;
    bool moveLeft = false;
    bool moveForward = false;
    bool moveBackward = false;
    bool moveUp = false;
    bool moveDown = false;

    // movement speed
    const float moveSpeed = 5.0f;
    
    // mouse sensitivity
    const float sensitivity = 0.1f;

    // last frame timestamp
    DWORD lastFrame = timeGetTime();
};
