#pragma once

#include <memory>
#include <array>
#include <vector>
#include <chrono>
#include <string>
#include <d3d11_1.h>
#include <directxmath.h>

#include "camera.h"
#include "shader.h"
#include "spotlight.h"


using namespace DirectX;

class Primitive;
class Unit;

template<typename T>
class ConstBuffer;

class Graphics {
public:
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

    struct MaterialConstantBuffer
    {
        XMMATRIX World;
        XMFLOAT3 F0;
        float roughness;
        float metalness;
        float _dummy[3];
    };

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

    void startEvent(LPCWSTR eventName);
    void endEvent();

    std::shared_ptr<ConstBuffer<SimpleConstantBuffer>> getSimpleCbuf() const;
    std::shared_ptr<ConstBuffer<MaterialConstantBuffer>> getMaterialCbuf() const;

    std::shared_ptr<Shader> getPBRShader() const;
    Camera& getCamera();
    ID3D11SamplerState* getSamplerState() const;

    void addUnit(std::shared_ptr<Unit>);
    bool makeSRVFromFile(std::string const &texFileName, ID3D11ShaderResourceView *&srv);

private:
    void renderScene();
    void moveCamera();
    void renderGUI();

    bool evalMeanBrightnessTex(ID3D11ShaderResourceView*& srv, ID3D11Texture2D*& tex);
    float calcMeanBrightness(ID3D11Texture2D* brightnessPixelTex2D);

    bool initIrradianceMap();
    void buildIrradianceMap();

    bool createDepthStencil(UINT width, UINT height);

    bool createRenderTargetTexture(UINT width, UINT height, 
        ID3D11RenderTargetView** rtv, ID3D11ShaderResourceView*& srv,
        ID3D11SamplerState*& samplerState, 
        DXGI_FORMAT format, bool isCubic, bool createSamplerState = false,
        ID3D11Texture2D **tex = nullptr);

    bool createCPUAccessedTexture(ID3D11Texture2D*& dst, ID3D11Texture2D* src);

    void setViewport(UINT width, UINT height);
    void setRenderTarget(ID3D11RenderTargetView* rtv, bool useDSV = true);

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
    std::array<ID3D11RenderTargetView*, 6> cubeRTV;

    ID3D11ShaderResourceView* baseSRV = nullptr;
    ID3D11ShaderResourceView* skySphereSRV = nullptr;    
    ID3D11ShaderResourceView* cubeSRV = nullptr;


    ID3D11SamplerState* samplerState = nullptr;

    //------------//
    ID3DUserDefinedAnnotation* annotation = nullptr;

    struct PBRConstantBuffer
    {
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

    struct ScreenSpaceConstantBuffer
    {
        int isScreenSpace;
        int _dummy[3];
    };

    Camera camera;

    std::shared_ptr<Shader> 
        pbrShader, brightShader,
        tonemapShader, texShader,
        cylinder2cubemapShader;
    //std::unique_ptr<Primitive> quadPrim;
    std::shared_ptr<Primitive> 
        screenQuadPrim, brightQuadPrim, skySpherePrim;
    std::array<std::shared_ptr<Primitive>, 6> cubemapPrim;

    std::shared_ptr<ConstBuffer<SimpleConstantBuffer>> simpleCbuf;
    std::shared_ptr<ConstBuffer<MaterialConstantBuffer>> materialCbuf;
    std::unique_ptr<ConstBuffer<PBRConstantBuffer>> pbrCbuf;
    std::unique_ptr<ConstBuffer<BrightnessConstantBuffer>> brightnessCbuf;
    std::unique_ptr<ConstBuffer<TonemapConstantBuffer>> tonemapCbuf;
    std::unique_ptr<ConstBuffer<ScreenSpaceConstantBuffer>> screenSpaceCbuf;

    std::array<SpotLight, 3> spotLights;
    std::chrono::system_clock::time_point start;

    std::vector<std::shared_ptr<Unit>> units;

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

    // sky cubemap tex width
    const UINT cubemapTexWidth = 512;

    // last frame timestamp
    DWORD lastFrame = timeGetTime();

    UINT width, height;
};
