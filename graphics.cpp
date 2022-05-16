#include <vector>
#include <string>
#include <d3d11_1.h>
#include <directxcolors.h>
#include <chrono>
#include <cmath>
#include <tuple>
#include <algorithm>
#include "DDSTextureLoader.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "graphics.h"
#include "camera.h"
#include "shader.h"
#include "primitive.h"
#include "spotlight.h"
#include "const_buffer.h"
#include "primitive_samples.h"
#include "unit.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#pragma comment(lib, "DirectXTK.lib")


std::shared_ptr<Graphics> Graphics::inst(new Graphics);


std::shared_ptr<Graphics> Graphics::init(HWND hWnd)
{
    start = std::chrono::system_clock::now();

    RECT rc;
    GetClientRect(hWnd, &rc);
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    std::vector<D3D_DRIVER_TYPE> driverTypes{
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    std::vector<D3D_FEATURE_LEVEL> featureLevels{
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };

    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    HRESULT hr = S_OK;
    for (auto &driverType: driverTypes) {
        hr = D3D11CreateDevice(
        nullptr, driverType, nullptr,
        createDeviceFlags, featureLevels.data(), static_cast<UINT>(featureLevels.size()),
        D3D11_SDK_VERSION, &device, &featureLevel, &context);
        
        if (FAILED(hr)) {
            // DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
            hr = D3D11CreateDevice(
                nullptr, driverType, nullptr,
                createDeviceFlags, featureLevels.data() + 1, static_cast<UINT>(featureLevels.size() - 1),
                D3D11_SDK_VERSION, &device, &featureLevel, &context);
        }

        if (SUCCEEDED(hr)) {
            break;
        }
    }
    if (FAILED(hr)) {
        return nullptr;
    }

    hr = context->QueryInterface(__uuidof(ID3DUserDefinedAnnotation),
        reinterpret_cast<void**>(&annotation));

    if (FAILED(hr))
        return nullptr;

    // Obtain DXGI factory from device (since we used nullptr for pAdapter above)
    IDXGIFactory1* dxgiFactory = nullptr;
    {
        IDXGIDevice* dxgiDevice = nullptr;
        hr = device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));

        if (SUCCEEDED(hr)) {
            IDXGIAdapter* adapter = nullptr;
            hr = dxgiDevice->GetAdapter(&adapter);

            if (SUCCEEDED(hr)) {
                hr = adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory));
                adapter->Release();
            }

            dxgiDevice->Release();
        }
    }

    if (FAILED(hr))
        return nullptr;

    // Create swap chain
    IDXGIFactory2 *dxgiFactory2 = nullptr;
    hr = dxgiFactory->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2));
    if (dxgiFactory2) {
        // DirectX 11.1 or later
        hr = device->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void**>(&device1));
        if (SUCCEEDED(hr))
            (void)context->QueryInterface( __uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&context1));

        DXGI_SWAP_CHAIN_DESC1 sd;
        ZeroMemory(&sd, sizeof(sd));
        sd.Width = width;
        sd.Height = height;
        sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.BufferCount = 1;

        hr = dxgiFactory2->CreateSwapChainForHwnd(device, hWnd, &sd, nullptr, nullptr, &swapChain1);
        if (SUCCEEDED(hr))
            hr = swapChain1->QueryInterface( __uuidof(IDXGISwapChain), reinterpret_cast<void**>(&swapChain));

        dxgiFactory2->Release();
    }
    else {
        // DirectX 11.0 systems
        DXGI_SWAP_CHAIN_DESC sd;
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = 1;
        sd.BufferDesc.Width = width;
        sd.BufferDesc.Height = height;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hWnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;

        hr = dxgiFactory->CreateSwapChain(device, &sd, &swapChain);
    }

    // Note this tutorial doesn't handle full-screen swapchains so we block the ALT+ENTER shortcut
    dxgiFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
    dxgiFactory->Release();

    if (FAILED(hr))
        return nullptr;

    initShaders();
    
    if (!createRenderTargetTexture(
        width, height, &baseTextureRTV, baseSRV, samplerState, DXGI_FORMAT_R32G32B32A32_FLOAT, false, true))
        return nullptr; 

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
    if (FAILED(hr))
        return nullptr;

    hr = device->CreateRenderTargetView(pBackBuffer, nullptr, &swapChainRTV);
    pBackBuffer->Release();
    if (FAILED(hr))
        return nullptr;

    if (!createDepthStencil(width, height))
        return nullptr;

    initGUI(hWnd);

    if (!initGeometry())
        return nullptr;

    initLights();

    width = width;
    height = height;

    return inst;
}

bool Graphics::createDepthStencil(UINT width, UINT height)
{
    ID3D11Texture2D* pDepthStencil = nullptr;
    ID3D11DepthStencilState* dsState = nullptr;

    D3D11_TEXTURE2D_DESC descDepth;

    ZeroMemory(&descDepth, sizeof(D3D11_TEXTURE2D_DESC));
    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;

    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags = 0;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.MiscFlags = 0;
    auto hr = inst->device->CreateTexture2D(&descDepth, nullptr, &pDepthStencil);

    if (FAILED(hr))
        return false;

    D3D11_DEPTH_STENCIL_DESC dsDesc;

    ZeroMemory(&dsDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
    // Depth test parameters
    dsDesc.DepthEnable = true;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

    // Stencil test parameters
    dsDesc.StencilEnable = true;
    dsDesc.StencilReadMask = 0xFF;
    dsDesc.StencilWriteMask = 0xFF;

    // Stencil operations if pixel is front-facing
    dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
    dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    // Stencil operations if pixel is back-facing
    dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
    dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    // Create depth stencil state
    hr = inst->device->CreateDepthStencilState(&dsDesc, &dsState);

    if (FAILED(hr))
        return false;

    // Bind depth stencil state
    inst->context->OMSetDepthStencilState(dsState, 1);

    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    ZeroMemory(&descDSV, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));

    descDSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

    // Create the depth stencil view
    hr = inst->device->CreateDepthStencilView(pDepthStencil, // Depth stencil texture
        &descDSV, // Depth stencil desc
        &dsv);  // [out] Depth stencil view

    dsState->Release();
    pDepthStencil->Release();

    return SUCCEEDED(hr);
}

void Graphics::initShaders()
{
    // Create constant buffers
    simpleCbuf = std::make_unique<ConstBuffer<SimpleConstantBuffer>>();
    pbrCbuf = std::make_unique<ConstBuffer<PBRConstantBuffer>>();
    materialCbuf = std::make_unique<ConstBuffer<MaterialConstantBuffer>>();
    brightnessCbuf = std::make_unique<ConstBuffer<BrightnessConstantBuffer>>();
    tonemapCbuf = std::make_unique<ConstBuffer<TonemapConstantBuffer>>();

    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC simpleLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    /*graphics->simpleShader = ShaderFactory::makeShaders(L"simple.fx", simpleLayout, 3);
    graphics->simpleShader->addConstBuffers({ { graphics->simpleCbuf->appliedConstBuffer(), true, true } });*/

    pbrShader = ShaderFactory::makeShaders(L"pbr.fx", simpleLayout, 3);
    pbrShader->addConstBuffers(
        { 
            { pbrCbuf->appliedConstBuffer(), true, true },
            { materialCbuf->appliedConstBuffer(), true, true }
        });

    D3D11_INPUT_ELEMENT_DESC brightLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    brightShader = ShaderFactory::makeShaders(L"brightness.fx", brightLayout, 2);
    brightShader->addConstBuffers({ { brightnessCbuf->appliedConstBuffer(), false, true } });

    tonemapShader = ShaderFactory::makeShaders(L"tonemap.fx", brightLayout, 2);
    tonemapShader->addConstBuffers({ { tonemapCbuf->appliedConstBuffer(), false, true } });

}

void Graphics::initLights()
{
    spotLights[0] = SpotLight(XMFLOAT3(-2, 0, 0), XMFLOAT3(0, 0, 1), XMFLOAT3(1, 0, 0), 15.0f, 1.0f);
    spotLights[1] = SpotLight(XMFLOAT3(2, 0, 0), XMFLOAT3(0, 0, 1), XMFLOAT3(1, 0, 0), 15.0f, 1.0f);
    spotLights[2] = SpotLight(XMFLOAT3(0, 3, 0), XMFLOAT3(0, 0, 1), XMFLOAT3(1, 0, 0), 15.0f, 1.0f);
}

std::shared_ptr<Graphics> Graphics::get()
{
    return inst;
}

void Graphics::setViewport(UINT width, UINT height)
{
    // Setup the viewport
    D3D11_VIEWPORT vp;
    ZeroMemory(&vp, sizeof(D3D11_VIEWPORT));
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    inst->context->RSSetViewports(1, &vp);
}

bool Graphics::createRenderTargetTexture(
    UINT width, UINT height, ID3D11RenderTargetView** rtv,
    ID3D11ShaderResourceView*& srv, 
    ID3D11SamplerState*& samplerState, 
    DXGI_FORMAT format, bool isCubic, bool createSamplerState,
    ID3D11Texture2D** tex)
{
    // Setup render target texture
    D3D11_TEXTURE2D_DESC td;
    ZeroMemory(&td, sizeof(td));
    td.Width = width;
    td.Height = height;
    td.MipLevels = 1;
    td.ArraySize = isCubic ? 6 : 1;
    td.Format = format;
    td.SampleDesc.Count = 1;
    td.SampleDesc.Quality = 0;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    td.CPUAccessFlags = 0;
    if (isCubic)
        td.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

    ID3D11Texture2D* rndTargetTexWH = nullptr;
    auto hr = inst->device->CreateTexture2D(&td, NULL, &rndTargetTexWH);
    if (FAILED(hr))
        return false;

    if (tex)
        *tex = rndTargetTexWH;

    // Setup the description of the render target view.
    D3D11_RENDER_TARGET_VIEW_DESC rtvd;

    ZeroMemory(&rtvd, sizeof(D3D11_RENDER_TARGET_VIEW_DESC));
    rtvd.Format = td.Format;
    rtvd.ViewDimension = isCubic ? D3D11_RTV_DIMENSION_TEXTURE2DARRAY : D3D11_RTV_DIMENSION_TEXTURE2D;
    if (isCubic)
        rtvd.Texture2DArray.ArraySize = 1;
    else
        rtvd.Texture2D.MipSlice = 0;

    if (!isCubic)
    {
        hr = inst->device->CreateRenderTargetView(rndTargetTexWH, &rtvd, rtv);
        if (FAILED(hr))
            return false;
    }
    else
        for (UINT i = 0; i < 6; i++)
        {
            // Change slot RTV desc to choose correct slice from array
            rtvd.Texture2DArray.FirstArraySlice = D3D11CalcSubresource(0, i, 1);

            // Create the RTV for the slot in m_renderSlotsTexArray
            hr = device->CreateRenderTargetView(rndTargetTexWH, &rtvd, &rtv[i]);

            if (FAILED(hr))
                return false;
        }

    if (createSamplerState)
    {
        // Create the sample state
        D3D11_SAMPLER_DESC sampDesc;
        ZeroMemory(&sampDesc, sizeof(sampDesc));
        sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        sampDesc.MinLOD = 0;
        sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
        hr = inst->device->CreateSamplerState(&sampDesc, &samplerState);
        if (FAILED(hr))
            return false;
    }

    // Setup the description of the shader resource view.
    D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
    srvd.Format = td.Format;
    srvd.ViewDimension = isCubic ? D3D11_SRV_DIMENSION_TEXTURECUBE : D3D11_SRV_DIMENSION_TEXTURE2D;
    srvd.Texture2D.MostDetailedMip = 0;
    srvd.Texture2D.MipLevels = 1;

    // Create the shader resource view.
    hr = inst->device->CreateShaderResourceView(rndTargetTexWH, &srvd, &srv);

    if (!tex)
        rndTargetTexWH->Release();

    if (FAILED(hr))
        return false;

    return true;
}

bool Graphics::createCPUAccessedTexture(ID3D11Texture2D*& dst, ID3D11Texture2D* src) {
    // Setup render target texture
    D3D11_TEXTURE2D_DESC td;

    src->GetDesc(&td);
    td.Usage = D3D11_USAGE_STAGING;
    td.BindFlags = 0;
    //td.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    auto hr = inst->device->CreateTexture2D(&td, NULL, &dst);
    if (FAILED(hr))
        return false;

    context->CopyResource(dst, src);
    return true;
}


bool Graphics::initGeometry() {
    bool success = true;

    for (auto& u : units)
        u->init(inst);

    //success &= createQuad();
    success &= PrimitiveSample::createScreenQuad(screenQuadPrim, true);
    success &= PrimitiveSample::createScreenQuad(brightQuadPrim, false, 0.8f);

    initIrradianceMap();
    buildIrradianceMap();

    return success;
}


void Graphics::initGUI(HWND hWnd) {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX11_Init(inst->device, inst->context);
}


void Graphics::moveCamera() {
    // Camera movement
    XMVECTOR moveDirection = { 0.0f, 0.0f, 0.0f, 0.0f };
    if (moveRight) moveDirection += camera.getRight();
    if (moveLeft) moveDirection -= camera.getRight();
    if (moveForward) moveDirection += camera.getDirection();
    if (moveBackward) moveDirection -= camera.getDirection();
    if (moveUp) moveDirection += { 0.0f, 1.0f, 0.0f, 0.0f };
    if (moveDown) moveDirection += { 0.0f, -1.0f, 0.0f, 0.0f };

    deltaTime = (timeGetTime() - lastFrame) / 1000.0f;
    moveDirection *= moveSpeed * deltaTime;

    if (XMVectorGetX(XMVector3Length(moveDirection)) > 1e-4) {
        camera.move(moveDirection);
    }

    lastFrame = timeGetTime();
}


void Graphics::startEvent(LPCWSTR eventName)
{
#ifdef _DEBUG
    annotation->BeginEvent(eventName);
#endif
}


void Graphics::endEvent()
{
#ifdef _DEBUG
    annotation->EndEvent();
#endif
}

void Graphics::renderScene() {
    for (auto& u : units)
        u->prepareForRender(inst);

    // Setup common constant buffer
    PBRConstantBuffer pbrCB;
    ZeroMemory(&pbrCB, sizeof(PBRConstantBuffer));
    pbrCB.DrawMask = DrawMask;
    pbrCB.View = XMMatrixTranspose(camera.view());
    pbrCB.Projection = XMMatrixTranspose(camera.projection());

    // Setup lights
    for (size_t idx = 0; idx < spotLights.size(); idx++) {
        pbrCB.LightPos[idx] = spotLights[idx].getPosition();
        pbrCB.LightColor[idx] = spotLights[idx].getColor();
        pbrCB.LightIntensity[idx] = spotLights[idx].getIntensity();
    }
    auto pos = camera.getPosition().m128_f32;
    pbrCB.CameraPos = XMFLOAT3(pos[0], pos[1], pos[2]);

    pbrCbuf->update(pbrCB);

    for (auto& u : units)
        u->render(inst);

    // Draw skybox
    SimpleConstantBuffer simpleCB;

    startEvent(L"DrawSkybox");
    const float scale = 3000.0f;
    simpleCB.mWorld = XMMatrixTranspose(XMMatrixMultiply(XMMatrixScaling(scale, scale, scale), XMMatrixTranslation(pos[0], pos[1], pos[2])));
    simpleCB.mProjection = XMMatrixTranspose(camera.projection());
    simpleCB.mView = XMMatrixTranspose(camera.view());

    simpleCbuf->update(simpleCB);
    skyboxPrim->render(skyboxShader, samplerState, cubeSRV);
    endEvent();
}

void Graphics::renderGUI() {
    // Start the Dear ImGui frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("PBR Setting");

    ImGui::Text("Render mode");

    if (ImGui::RadioButton("Complete scene", DrawMask == 0))
        DrawMask = 0;
    if (ImGui::RadioButton("Normal Distributional Function", DrawMask == 1))
        DrawMask = 1;
    if (ImGui::RadioButton("Fresnel Function", DrawMask == 2))
        DrawMask = 2;
    if (ImGui::RadioButton("Geometry Function", DrawMask == 3))
        DrawMask = 3;

    ImGui::End();

    ImGui::Render();
}

void Graphics::setRenderTarget(ID3D11RenderTargetView* rtv, bool useDSV)
{
    float clearColor[] = { 0.3f, 0.5f, 0.7f, 1.0f };
    context->ClearRenderTargetView(rtv, clearColor);
    if (useDSV)
        context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    context->OMSetRenderTargets(1, &rtv, useDSV ? inst->dsv : nullptr);
}

bool Graphics::evalMeanBrightnessTex(ID3D11ShaderResourceView*&srv, ID3D11Texture2D*& tex)
{
    // eval brightness
    ID3D11RenderTargetView* rtv_bright = nullptr;
    ID3D11ShaderResourceView* srv_bright = nullptr;
    ID3D11Texture2D* tex_bright = nullptr;

    startEvent(L"DrawScreenQuadEvalBrightness");
    if (!createRenderTargetTexture(
        width, height, &rtv_bright, srv_bright, 
        samplerState, DXGI_FORMAT_R32_FLOAT, false, false))
        return false;

    BrightnessConstantBuffer cb;
    ZeroMemory(&cb, sizeof(BrightnessConstantBuffer));

    setViewport(width, height);
    setRenderTarget(rtv_bright);
    cb.isBrightnessCalc = 1;
    brightnessCbuf->update(cb);
    screenQuadPrim->render(brightShader, samplerState, baseSRV);
    endEvent();
    rtv_bright->Release();

    // 2 ^ n
    auto n = static_cast<int>(std::max<double>(std::ceil(std::log2(width)), std::ceil(std::log(height))));
    auto two_pow_n = 1 << n;
    ID3D11ShaderResourceView* curSRV = srv_bright, *prevSRV = baseSRV;
    ID3D11Texture2D* resTex2D = nullptr;

    cb.isBrightnessCalc = 0;
    brightnessCbuf->update(cb);
    for (; n >= 0; n--, two_pow_n >>= 1)
    {
        ID3D11RenderTargetView* rtv_2n = nullptr;
        ID3D11ShaderResourceView* srv_2n = nullptr;
        bool cpuAccessFlag = n == 0;
        ID3D11Texture2D* tex_2n = nullptr;

        startEvent((std::wstring(L"DrawScreenQuad2^") + std::to_wstring(n)).c_str());
        if (!createRenderTargetTexture(
            two_pow_n, two_pow_n, &rtv_2n, srv_2n, samplerState, 
            DXGI_FORMAT_R32_FLOAT, false, false, cpuAccessFlag ? &tex_2n : nullptr))
            return false;

        setViewport(two_pow_n, two_pow_n);
        setRenderTarget(rtv_2n, false);
        screenQuadPrim->render(brightShader, samplerState, curSRV);
        endEvent();

        if (cpuAccessFlag)
        {
            if (!createCPUAccessedTexture(resTex2D, tex_2n))
                return false;
            tex_2n->Release();
        }

        prevSRV = curSRV;
        curSRV = srv_2n;

        rtv_2n->Release();
        if (prevSRV != baseSRV)
            prevSRV->Release();
    }
    srv = curSRV;
    tex = resTex2D;
    return true;
}

float Graphics::calcMeanBrightness(ID3D11Texture2D* brightnessPixelTex2D) {
    D3D11_MAPPED_SUBRESOURCE subrc;
    auto hr = context->Map(brightnessPixelTex2D, 0, D3D11_MAP_READ, 0, &subrc);
    if (FAILED(hr))
        printf("Failed map resource :(");

    float* arr = (float*)subrc.pData;

    // Check it is really tex 1x1 pixel
    assert(subrc.DepthPitch >= 1);
    assert(subrc.RowPitch >= 1);
    //assert(fabs(arr[0] - arr[1]) < 1e-6);
    //assert(fabs(arr[1] - arr[2]) < 1e-6);
    //assert(fabs(arr[3] - 1) < 1e-6);
    context->Unmap(brightnessPixelTex2D, 0);

    return std::exp(arr[0]) - 1.0f;
}

bool Graphics::makeSRVFromFile(std::string const& texFileName, ID3D11ShaderResourceView*& srv)
{
    int width, height, comps;
    auto data = stbi_load(texFileName.c_str(), &width, &height, &comps, 0);

    float* float_data = new float[width * height * 4];

    if (comps == 3)
        for (int i = 0; i < width * height; i++)
        {
            float_data[i * 4 + 0] = data[i * 3 + 0] / 255.0f;
            float_data[i * 4 + 1] = data[i * 3 + 1] / 255.0f;
            float_data[i * 4 + 2] = data[i * 3 + 2] / 255.0f;
            float_data[i * 4 + 3] = 1.0f;
        }
    else if (comps == 4)
        for (int i = 0; i < width * height * 4; i++)
            float_data[i] = data[i] / 255.0f;
    else
        assert(false); // not supported

    D3D11_TEXTURE2D_DESC td;
    ZeroMemory(&td, sizeof(td));
    td.Width = width;
    td.Height = height;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    td.SampleDesc.Count = 1;
    td.SampleDesc.Quality = 0;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    td.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA sd;
    ZeroMemory(&sd, sizeof(D3D11_SUBRESOURCE_DATA));

    sd.pSysMem = float_data;
    sd.SysMemPitch = sizeof(float) * width * 4;

    ID3D11Texture2D* tex = nullptr;
    auto hr = inst->device->CreateTexture2D(&td, &sd, &tex);
    if (FAILED(hr))
    {
        delete[] float_data;
        stbi_image_free(data);
        return false;
    }

    // Setup the description of the shader resource view.
    D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
    srvd.Format = td.Format;
    srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvd.Texture2D.MostDetailedMip = 0;
    srvd.Texture2D.MipLevels = 1;

    // Create the shader resource view.
    hr = inst->device->CreateShaderResourceView(tex, &srvd, &srv);
    tex->Release();

    if (FAILED(hr))
    {
        delete[] float_data;
        stbi_image_free(data);
        return false;
    }

    delete[] float_data;
    stbi_image_free(data);

    return true;
}

bool Graphics::initIrradianceMap()
{
    D3D11_INPUT_ELEMENT_DESC simpleLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    cylinder2cubemapShader = ShaderFactory::makeShaders(L"cylinder2cubemap.fx", simpleLayout, 3);
    cylinder2cubemapShader->addConstBuffers(
        {
            { simpleCbuf->appliedConstBuffer(), true, true }
        });

    //"je_gray_park_4k.hdr"
    if (!makeSRVFromFile("Road_to_MonumentValley_Ref.hdr", skySphereSRV))
        return false;

    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC texLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    texShader = ShaderFactory::makeShaders(L"texture.fx", texLayout, 2);
    texShader->addConstBuffers(
        {
            { simpleCbuf->appliedConstBuffer(), true, false }
        });

    skyboxShader = ShaderFactory::makeShaders(L"skybox.fx", simpleLayout, 3);
    skyboxShader->addConstBuffers(
        {
            { simpleCbuf->appliedConstBuffer(), true, false}
        });

    //if (!PrimitiveSample::createSphere(skySpherePrim, 500.0f, true, true))
    //    return false;

    if (!PrimitiveSample::createCube(skyboxPrim, true))
        return false;

    // Create cubemap texture
    if (!createRenderTargetTexture(cubemapTexWidth, cubemapTexWidth, cubeRTV.data(), cubeSRV, samplerState, DXGI_FORMAT_R32G32B32A32_FLOAT, true))
        return false;

    XMFLOAT3 quadPos[6][4] =
    {
        {XMFLOAT3(0.5, -0.5, 0.5), XMFLOAT3(0.5, -0.5, -0.5), XMFLOAT3(0.5, 0.5, -0.5), XMFLOAT3(0.5, 0.5, 0.5)},        // +x
        {XMFLOAT3(-0.5, -0.5, -0.5), XMFLOAT3(-0.5, -0.5, 0.5), XMFLOAT3(-0.5, 0.5, 0.5), XMFLOAT3(-0.5, 0.5, -0.5)},    // -x
        {XMFLOAT3(-0.5, 0.5, 0.5), XMFLOAT3(0.5, 0.5, 0.5), XMFLOAT3(0.5, 0.5, -0.5), XMFLOAT3(-0.5, 0.5, -0.5)},        // +y
        {XMFLOAT3(0.5, -0.5, 0.5), XMFLOAT3(-0.5, -0.5, 0.5), XMFLOAT3(-0.5, -0.5, -0.5), XMFLOAT3(0.5, -0.5, -0.5)},    // -y
        {XMFLOAT3(-0.5, -0.5, 0.5), XMFLOAT3(0.5, -0.5, 0.5), XMFLOAT3(0.5, 0.5, 0.5), XMFLOAT3(-0.5, 0.5, 0.5)},        // +z
        {XMFLOAT3(0.5, -0.5, -0.5), XMFLOAT3(-0.5, -0.5, -0.5), XMFLOAT3(-0.5, 0.5, -0.5), XMFLOAT3(0.5, 0.5, -0.5)}     // -z
    };

    for (size_t quad = 0; quad < 6; quad++)
    {
        std::array<SimpleVertex, 4> vertices;
        std::generate(vertices.begin(), vertices.end(),
            [vertex = 0, quadPos, quad]() mutable {
            return SimpleVertex{ quadPos[quad][vertex++], XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) };
        });
        if (!PrimitiveSample::createQuad<SimpleVertex>(cubemapPrim[quad], vertices))
            return false;
    }
    
    return true;
}

void Graphics::buildIrradianceMap()
{
    Camera skyCamera({ 0.0f, 0.0f, 0.0f });
    auto pos = skyCamera.getPosition().m128_f32;
    SimpleConstantBuffer simpleCB;
    simpleCB.mWorld = XMMatrixIdentity();
    simpleCB.mProjection = XMMatrixTranspose(skyCamera.projection());

    XMFLOAT3 direction[] =
    {
        XMFLOAT3(1, 0, 0),
        XMFLOAT3(-1, 0, 0),
        XMFLOAT3(0, 1, 0),
        XMFLOAT3(0, -1, 0),
        XMFLOAT3(0, 0, 1),
        XMFLOAT3(0, 0, -1)
    };

    XMFLOAT3 right[] =
    {
        XMFLOAT3(0, 0, -1),
        XMFLOAT3(0, 0, 1),
        XMFLOAT3(1, 0, 0),
        XMFLOAT3(1, 0, 0),
        XMFLOAT3(1, 0, 0),
        XMFLOAT3(-1, 0, 0)
    };

    ID3D11RenderTargetView* nullRTV[] = { nullptr };
    ID3D11ShaderResourceView* nullSRV[] = { nullptr };

    for (size_t i = 0; i < 6; i++)
    {
        startEvent((std::wstring(L"SkySphere2Cube") + std::to_wstring(i)).c_str());
        setViewport(cubemapTexWidth, cubemapTexWidth);
        setRenderTarget(cubeRTV[i], false);

        skyCamera.updateViewMatrix(XMLoadFloat3(&direction[i]), XMLoadFloat3(&right[i]));
        simpleCB.mView = XMMatrixTranspose(skyCamera.view());
        simpleCbuf->update(simpleCB);
        cubemapPrim[i]->render(cylinder2cubemapShader, samplerState, skySphereSRV);

        context->PSSetShaderResources(0, 1, nullSRV);

        endEvent();
    }
}

void Graphics::render()
{
    moveCamera();
    renderGUI();

    // TODO just for debug. Remove later.
    buildIrradianceMap();

    if (DrawMask == 0)
    {
        setViewport(width, height);
        setRenderTarget(baseTextureRTV);
        //skySpherePrim->render(texShader, samplerState, skySphereSRV);
        renderScene();

        ID3D11ShaderResourceView* brightnessPixelSRV = nullptr;
        ID3D11Texture2D* brightnessPixelTex2D = nullptr;

        bool brightness_ok = evalMeanBrightnessTex(brightnessPixelSRV, brightnessPixelTex2D);
        if (!brightness_ok)
            printf("Failed eval mean brightness :(");
        auto meanBrightness = calcMeanBrightness(brightnessPixelTex2D);

        const float adaptationTime = 1.5f;
        float curMeanBrightness;
        if (std::fabs(prevMeanBrightness + 1) > 1e-6)
            curMeanBrightness = prevMeanBrightness + (meanBrightness - prevMeanBrightness) * (1 - exp(-deltaTime / adaptationTime));
        else
            curMeanBrightness = meanBrightness;
        prevMeanBrightness = curMeanBrightness;

        setViewport(width, height);
        setRenderTarget(swapChainRTV);

        TonemapConstantBuffer cb;
        ZeroMemory(&cb, sizeof(TonemapConstantBuffer));
        cb.meanBrightness = curMeanBrightness;

        startEvent(L"DrawScreenQuad");
        cb.isBrightnessWindow = 0;
        tonemapCbuf->update(cb);
        screenQuadPrim->render(tonemapShader, samplerState, baseSRV);
        endEvent();

        startEvent(L"DrawBrightQuad");
        cb.isBrightnessWindow = 1;
        tonemapCbuf->update(cb);
        brightQuadPrim->render(tonemapShader, samplerState, brightnessPixelSRV);
        endEvent();

        if (brightnessPixelSRV)
            brightnessPixelSRV->Release();
        if (brightnessPixelTex2D)
            brightnessPixelTex2D->Release();
    }
    else
    {
        setViewport(width, height);
        setRenderTarget(swapChainRTV);
        renderScene();
    }

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    swapChain->Present(0, 0);

    // Unbind shader resource
    ID3D11ShaderResourceView* views[1] = { nullptr };
    context->PSSetShaderResources(0, 1, views);
}

void Graphics::cleanup() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    for (auto& u : units)
        u->cleanup(inst);

    if (swapChainRTV) swapChainRTV->Release();
    if (baseTextureRTV) baseTextureRTV->Release();
    for (auto rtv : cubeRTV)
        if (rtv) rtv->Release();

    if (baseSRV) baseSRV->Release();
    if (skySphereSRV) skySphereSRV->Release();
    if (cubeSRV) cubeSRV->Release();

    //simpleShader->cleanup();
    pbrShader->cleanup();
    brightShader->cleanup();
    tonemapShader->cleanup();
    texShader->cleanup();
    skyboxShader->cleanup();
    cylinder2cubemapShader->cleanup();

    simpleCbuf->cleanup();
    pbrCbuf->cleanup();
    materialCbuf->cleanup();
    brightnessCbuf->cleanup();
    tonemapCbuf->cleanup();

    //quadPrim->cleanup();
    screenQuadPrim->cleanup();
    brightQuadPrim->cleanup();
    //skySpherePrim->cleanup();
    skyboxPrim->cleanup();

    for (auto prim : cubemapPrim)
        prim->cleanup();

    if (dsv) dsv->Release();

    if (swapChain1) swapChain1->Release();
    if (swapChain) swapChain->Release();
    if (annotation) annotation->Release();

    if (samplerState) samplerState->Release();

    if (context) context->ClearState();
    if (context1) context1->Release();
    if (context) context->Release();
    if (device1) device1->Release();

    ID3D11Debug* d3dDebug = nullptr;
    device->QueryInterface(IID_ID3D11Debug, reinterpret_cast<void**>(&d3dDebug));
    
    int refs = device->Release();

#ifdef _DEBUG
    if (refs > 1) {
        d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
    }
#endif
    d3dDebug->Release();
}

HRESULT Graphics::resizeBackbuffer(UINT width, UINT height) {
    
    HRESULT hr;
    ID3D11RenderTargetView* nullViews [] = { nullptr };

    camera.setAspectRatio(width / (FLOAT)height);

    context->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);

    if (swapChainRTV) swapChainRTV->Release();
    if (baseTextureRTV) baseTextureRTV->Release();
    if (baseSRV) baseSRV->Release();
    if (dsv) dsv->Release();
    context->Flush();

    hr = swapChain->ResizeBuffers(1, width, height, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 0);
    if (FAILED(hr))
        return hr;

    ID3D11Texture2D* backBuffer = nullptr;
    hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
    if (FAILED(hr))
        return hr;

    hr = device->CreateRenderTargetView(backBuffer, nullptr, &swapChainRTV);
    if (FAILED(hr))
        return hr;
 
    backBuffer->Release();

    if (!inst->createRenderTargetTexture(
        width, height, &baseTextureRTV, baseSRV, samplerState, DXGI_FORMAT_R32G32B32A32_FLOAT, false))
        return S_FALSE;

    if (!inst->createDepthStencil(width, height))
        return S_FALSE;;

    setViewport(width, height);

    this->width = width;
    this->height = height;
    
    return S_OK;
}

std::shared_ptr<ConstBuffer<Graphics::SimpleConstantBuffer>> Graphics::getSimpleCbuf() const
{
    return simpleCbuf;
}

std::shared_ptr<ConstBuffer<Graphics::MaterialConstantBuffer>> Graphics::getMaterialCbuf() const
{
    return materialCbuf;
}

std::shared_ptr<Shader> Graphics::getPBRShader() const
{
    return pbrShader;
}

Camera& Graphics::getCamera()
{
    return camera;
}

ID3D11SamplerState* Graphics::getSamplerState() const
{
    return samplerState;
}

void Graphics::addUnit(std::shared_ptr<Unit> unit)
{
    units.push_back(unit);
}

void Graphics::setMoveRight(bool move) { moveRight = move; }
void Graphics::setMoveLeft(bool move) { moveLeft = move; }
void Graphics::setMoveForward(bool move) { moveForward = move; }
void Graphics::setMoveBackward(bool move) { moveBackward = move; }
void Graphics::setMoveUp(bool move) { moveUp = move; }
void Graphics::setMoveDown(bool move) { moveDown = move; }

void Graphics::rotate(int mouseDeltaX, int mouseDeltaY) {
    camera.rotate(mouseDeltaX * sensitivity, mouseDeltaY * sensitivity);
}

void Graphics::resetLightIntensity(int lightIndex) {
    spotLights[lightIndex].setIntensity(1.0f);
}

void Graphics::increaseLightIntensity(int lightIndex) {
    spotLights[lightIndex].setIntensity(min(spotLights[lightIndex].getIntensity() + 10.0f, 10000.0f));
}

void Graphics::decreaseLightIntensity(int lightIndex) {
    spotLights[lightIndex].setIntensity(max(spotLights[lightIndex].getIntensity() - 10.0f, 0.0f));
}
