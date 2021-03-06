#pragma once

#include <memory>
#include <array>
#include <vector>
#include <chrono>
#include <d3d11_1.h>
#include <directxmath.h>

#include "camera.h"
#include "shader.h"
#include "spotlight.h"


using namespace DirectX;

class Primitive;

template<typename T>
class ConstBuffer;

class Graphics {
public:
    // factory method
    static std::shared_ptr<Graphics> init(HWND hWnd);
    static std::shared_ptr<Graphics> get();

    void initShaders();
    void initLights();
    bool initGeometry();
    void initGUI(HWND hWnd);

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

    void resetLightIntensity(int lightIndex);
    void increaseLightIntensity(int lightIndex);
    void decreaseLightIntensity(int lightIndex);

private:
    void startEvent(LPCWSTR eventName);
    void endEvent();

    void moveCamera();
    void renderScene();
    void renderGUI();

    bool evalMeanBrightnessTex(ID3D11ShaderResourceView*& srv, ID3D11Texture2D*& tex);
    float calcMeanBrightness(ID3D11Texture2D* brightnessPixelTex2D);

    bool createDepthStencil(UINT width, UINT height);

    bool createRenderTargetTexture(UINT width, UINT height, 
        ID3D11RenderTargetView*& rtv, ID3D11ShaderResourceView*& srv,
        ID3D11SamplerState*& samplerState, 
        DXGI_FORMAT format, bool createSamplerState = false,
        ID3D11Texture2D **tex = nullptr);

    bool createCPUAccessedTexture(ID3D11Texture2D*& dst, ID3D11Texture2D* src);

    void setViewport(UINT width, UINT height);
    void setRenderTarget(ID3D11RenderTargetView* rtv, bool useDSV = true);

    bool createQuad(std::shared_ptr<Primitive>& prim);
    bool createScreenQuad(std::shared_ptr<Primitive> &prim, bool full, float val = 0.0f);
    bool createSphere(std::shared_ptr<Primitive>& prim, float R, bool invDir = false);
    bool createSkybox();

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
    ID3D11DepthStencilView* dsv = nullptr;

    ID3D11RenderTargetView* swapChainRTV = nullptr;
    ID3D11RenderTargetView* baseTextureRTV = nullptr;
    ID3D11ShaderResourceView* baseSRV = nullptr;
    ID3D11SamplerState* samplerState = nullptr;

    ID3D11SamplerState* skyboxSamplerState = nullptr;
    ID3D11ShaderResourceView* skyboxSRV = nullptr;

    //------------//
    ID3DUserDefinedAnnotation* annotation = nullptr;

    struct SimpleVertex
    {
        XMFLOAT3 Pos;
        XMFLOAT3 Norm;
        XMFLOAT4 Color;
    };

    struct TextureVertex
    {
        XMFLOAT3 Pos;
        XMFLOAT4 Color;
        XMFLOAT2 Tex;
    };

    struct SimpleConstantBuffer
    {
        XMMATRIX mWorld;
        XMMATRIX mView;
        XMMATRIX mProjection;
        XMFLOAT4 LightPos[4];
        XMFLOAT4 LightDir[4];
        float LightCutoff[4];
        float LightIntensity[4]; // only first component is used
    };

    struct PBRConstantBuffer
    {
        XMMATRIX World;
        XMMATRIX View;
        XMMATRIX Projection;
        // lights
        XMFLOAT4 LightColor[4];
        XMFLOAT4 LightPos[4];
        float LightIntensity[4];
        // camera
        XMFLOAT3 CameraPos;
        int DrawMask;
    };

    struct MaterialConstantBuffer
    {
        XMFLOAT3 F0;
        float roughness;
        float metalness;
        float _dummy[3];
    };

    struct TonemapConstantBuffer
    {
        int isBrightnessWindow;
        float meanBrightness;
        float _dummy[2];
    };

    struct BrightnessConstantBuffer
    {
        int isBrightnessCalc;
        float _dummy[3];
    };

    Camera camera;

    std::unique_ptr<Shader>
        /*simpleShader, */ pbrShader, brightShader, tonemapShader, skyboxShader;
    //std::unique_ptr<Primitive> quadPrim;
    std::shared_ptr<Primitive> 
        screenQuadPrim, brightQuadPrim,
        spherePrim, skyboxPrim;

    std::unique_ptr<ConstBuffer<SimpleConstantBuffer>> simpleCbuf;
    std::unique_ptr<ConstBuffer<PBRConstantBuffer>> pbrCbuf;
    std::unique_ptr<ConstBuffer<MaterialConstantBuffer>> materialCbuf;
    std::unique_ptr<ConstBuffer<BrightnessConstantBuffer>> brightnessCbuf;
    std::unique_ptr<ConstBuffer<TonemapConstantBuffer>> tonemapCbuf;

    std::array<SpotLight, 3> spotLights;

    std::chrono::system_clock::time_point start;

    float prevMeanBrightness = -1.0f;
    float deltaTime;

    // movement flags
    bool moveRight = false;
    bool moveLeft = false;
    bool moveForward = false;
    bool moveBackward = false;
    bool moveUp = false;
    bool moveDown = false;

    int DrawMask = 0;

    // movement speed
    const float moveSpeed = 15.0f;
    
    // mouse sensitivity
    const float sensitivity = 0.1f;

    const float radius = 2.0f;

    // last frame timestamp
    DWORD lastFrame = timeGetTime();

    UINT width, height;
};
