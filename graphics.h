#pragma once

#include <memory>
#include <vector>
#include <chrono>
#include <d3d11_1.h>
#include <directxmath.h>

#include "camera.h"
#include "shader.h"


using namespace DirectX;

class Primitive;

class Graphics {
public:
    // factory method
    static std::shared_ptr<Graphics> init(HWND hWnd);
    static std::shared_ptr<Graphics> get();

    bool initGeometry();

    ID3D11Device* getDevice() const { return device; }
    ID3D11DeviceContext* getContext() const { return context; }

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
    bool CreateRenderTargetTexture(UINT width, UINT height, ID3D11RenderTargetView*& rtv,
        ID3D11SamplerState*& samplerState, ID3D11ShaderResourceView*& srv);
    void setViewport(UINT width, UINT height);

    static std::shared_ptr<Graphics> inst;

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
    ID3D11RenderTargetView* baseTextureRTV = nullptr;
    ID3D11ShaderResourceView* baseSRV = nullptr;
    ID3D11SamplerState* samplerState = nullptr;

    //------------//
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
    };

    Camera camera;

    Shader simple, bright;
    std::unique_ptr<Primitive> cube;
    XMMATRIX world;

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
