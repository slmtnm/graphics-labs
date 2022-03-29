#include <vector>
#include <string>
#include <d3d11_1.h>
#include <directxcolors.h>
#include <chrono>
#include <cmath>
#include <tuple>
#include <algorithm>

#include "graphics.h"
#include "camera.h"
#include "shader.h"
#include "primitive.h"
#include "spotlight.h"
#include "const_buffer.h"


std::shared_ptr<Graphics> Graphics::inst(new Graphics);


std::shared_ptr<Graphics> Graphics::init(HWND hWnd) {
    // alias
    auto graphics = inst;

    graphics->start = std::chrono::system_clock::now();

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
        D3D11_SDK_VERSION, &graphics->device, &featureLevel, &graphics->context);
        
        if (FAILED(hr)) {
            // DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
            hr = D3D11CreateDevice(
                nullptr, driverType, nullptr,
                createDeviceFlags, featureLevels.data() + 1, static_cast<UINT>(featureLevels.size() - 1),
                D3D11_SDK_VERSION, &graphics->device, &featureLevel, &graphics->context);
        }

        if (SUCCEEDED(hr)) {
            break;
        }
    }
    if (FAILED(hr)) {
        return nullptr;
    }

    hr = graphics->context->QueryInterface(__uuidof(ID3DUserDefinedAnnotation),
        reinterpret_cast<void**>(&graphics->annotation));

    if (FAILED(hr))
        return nullptr;

    // Obtain DXGI factory from device (since we used nullptr for pAdapter above)
    IDXGIFactory1* dxgiFactory = nullptr;
    {
        IDXGIDevice* dxgiDevice = nullptr;
        hr = graphics->device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));

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
        hr = graphics->device->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void**>(&graphics->device1));
        if (SUCCEEDED(hr))
            (void)graphics->context->QueryInterface( __uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&graphics->context1));

        DXGI_SWAP_CHAIN_DESC1 sd;
        ZeroMemory(&sd, sizeof(sd));
        sd.Width = width;
        sd.Height = height;
        sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.BufferCount = 1;

        hr = dxgiFactory2->CreateSwapChainForHwnd(graphics->device, hWnd, &sd, nullptr, nullptr, &graphics->swapChain1);
        if (SUCCEEDED(hr))
            hr = graphics->swapChain1->QueryInterface( __uuidof(IDXGISwapChain), reinterpret_cast<void**>(&graphics->swapChain));

        dxgiFactory2->Release();
    }
    else {
        // DirectX 11.0 systems
        DXGI_SWAP_CHAIN_DESC sd;
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = 1;
        sd.BufferDesc.Width = width;
        sd.BufferDesc.Height = height;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hWnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;

        hr = dxgiFactory->CreateSwapChain(graphics->device, &sd, &graphics->swapChain);
    }

    // Note this tutorial doesn't handle full-screen swapchains so we block the ALT+ENTER shortcut
    dxgiFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
    dxgiFactory->Release();

    if (FAILED(hr))
        return nullptr;

    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC simpleLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    graphics->simpleShader.makeShaders(L"simple.fx", simpleLayout, 3);

    D3D11_INPUT_ELEMENT_DESC brightLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    graphics->brightShader.makeShaders(L"brightness.fx", brightLayout, 3);
    graphics->tonemapShader.makeShaders(L"tonemap.fx", brightLayout, 3);

    if (!graphics->createRenderTargetTexture(width, height, inst->baseTextureRTV, inst->baseSRV, inst->samplerState, true))
        return nullptr; 

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = graphics->swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
    if (FAILED(hr))
        return nullptr;

    hr = graphics->device->CreateRenderTargetView(pBackBuffer, nullptr, &graphics->swapChainRTV);
    pBackBuffer->Release();
    if (FAILED(hr))
        return nullptr;

    if (!graphics->initGeometry())
        return nullptr;

    graphics->width = width;
    graphics->height = height;

    return graphics;
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
    UINT width, UINT height, ID3D11RenderTargetView*& rtv,
    ID3D11ShaderResourceView*& srv, 
    ID3D11SamplerState*& samplerState, bool createSamplerState,
    ID3D11Texture2D** tex)
{
    // Setup render target texture
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
    td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    td.CPUAccessFlags = 0;

    ID3D11Texture2D* rndTargetTexWH = nullptr;
    auto hr = inst->device->CreateTexture2D(&td, NULL, &rndTargetTexWH);
    if (FAILED(hr))
        return false;

    if (tex)
        *tex = rndTargetTexWH;

    // Setup the description of the render target view.
    D3D11_RENDER_TARGET_VIEW_DESC rtvd;

    rtvd.Format = td.Format;
    rtvd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    rtvd.Texture2D.MipSlice = 0;

    hr = inst->device->CreateRenderTargetView(rndTargetTexWH, &rtvd, &rtv);
    if (FAILED(hr))
        return false;

    if (createSamplerState)
    {
        // Create the sample state
        D3D11_SAMPLER_DESC sampDesc;
        ZeroMemory(&sampDesc, sizeof(sampDesc));
        sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sampDesc.MinLOD = 0;
        sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
        hr = inst->device->CreateSamplerState(&sampDesc, &samplerState);
        if (FAILED(hr))
            return false;
    }

    // Setup the description of the shader resource view.
    D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
    srvd.Format = td.Format;
    srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
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


bool Graphics::createQuad()
{
    // Create vertex buffer
    auto color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
    SimpleVertex vertices[] =
    {
        { XMFLOAT3(-5.0f, -5.0f, 10.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), color},
        { XMFLOAT3(5.0f, -5.0f, 10.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), color},
        { XMFLOAT3(5.0f, 5.0f, 10.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), color},
        { XMFLOAT3(-5.0f, 5.0f, 10.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), color},
    };

    // Create index buffer
    UINT indices[] =
    {
        0, 2, 1,
        2, 0, 3
    };

    quadPrim = PrimitiveFactory::create<SimpleVertex>(vertices, 4, indices, 6);
    if (!quadPrim)
        return false;
    return true;
}


bool Graphics::createScreenQuad(std::shared_ptr<Primitive> &prim, bool full, float val) {
    // Create vertex buffer
    TextureVertex vertices[] =
    {
        { XMFLOAT3(-1.0f, full ? -1.0f : val, 0.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 1.0f)},
        { XMFLOAT3(full ? 1.0f : -val, full ? -1.0f : val, 0.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 1.0f)},
        { XMFLOAT3(full ? 1.0f : -val, 1.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 0.0f)},
        { XMFLOAT3(-1.0f, 1.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 0.0f)},
    };

    // Create index buffer
    UINT indices[] =
    {
        0, 2, 1,
        2, 0, 3
    };

    prim = PrimitiveFactory::create<TextureVertex>(vertices, 4, indices, 6);
    if (!prim)
        return false;
    return true;
}


bool Graphics::initGeometry() {
    bool success = true;
    success &= createQuad();
    success &= createScreenQuad(screenQuadPrim, true);
    success &= createScreenQuad(brightQuadPrim, false, 0.8f);

    simpleCbuf = std::make_unique<ConstBuffer<SimpleConstantBuffer>>();
    brightnessCbuf = std::make_unique<ConstBuffer<BrightnessConstantBuffer>>();
    tonemapCbuf = std::make_unique<ConstBuffer<TonemapConstantBuffer>>();

    return success;
}


void Graphics::moveCamera() {
    // Camera movement
    XMVECTOR moveDirection = { 0.0f, 0.0f, 0.0f, 0.0f };
    if (moveRight) moveDirection += camera.getRight();
    if (moveLeft) moveDirection += -camera.getRight();
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
    // Render quad
    SimpleConstantBuffer cb;
    ZeroMemory(&cb, sizeof(SimpleConstantBuffer));
    cb.mView = XMMatrixTranspose(camera.view());
    cb.mProjection = XMMatrixTranspose(camera.projection());
    cb.mWorld = XMMatrixIdentity();

    // Setup lights
    cb.LightPos[0] = XMFLOAT4(-2, 0, 0, 1);
    cb.LightPos[1] = XMFLOAT4(2, 0, 0, 1);
    cb.LightPos[2] = XMFLOAT4(0, 3, 0, 1);
    cb.LightPos[3] = XMFLOAT4(0, 5, 0, 1);

    cb.LightDir[0] = XMFLOAT4(0, 0, 1, 1);
    cb.LightDir[1] = XMFLOAT4(0, 0, 1, 1);
    cb.LightDir[2] = XMFLOAT4(0, 0, 1, 1);
    cb.LightDir[3] = XMFLOAT4(0, 0, 1, 1);

    cb.LightCutoff[0] = cosf(XMConvertToRadians(15));
    cb.LightCutoff[1] = cosf(XMConvertToRadians(15));
    cb.LightCutoff[2] = cosf(XMConvertToRadians(15));
    cb.LightCutoff[3] = cosf(XMConvertToRadians(10));

    cb.LightIntensity[0] = lightIntensity[0];
    cb.LightIntensity[1] = lightIntensity[1];
    cb.LightIntensity[2] = lightIntensity[2];

    startEvent(L"DrawQuad1");
    simpleCbuf->update(cb);
    quadPrim->render(simpleShader, { simpleCbuf->appliedConstBuffer() }, {0});
    endEvent();
}

void Graphics::setRenderTarget(ID3D11RenderTargetView* rtv)
{
    float clearColor[] = { 0.3f, 0.5f, 0.7f, 1.0f };
    context->ClearRenderTargetView(rtv, clearColor);
    context->OMSetRenderTargets(1, &rtv, nullptr);
}

bool Graphics::evalMeanBrightnessTex(ID3D11ShaderResourceView*&srv, ID3D11Texture2D*& tex)
{
    setViewport(width, height);
    setRenderTarget(baseTextureRTV);
    renderScene();

    // eval brightness
    ID3D11RenderTargetView* rtv_bright = nullptr;
    ID3D11ShaderResourceView* srv_bright = nullptr;
    ID3D11Texture2D* tex_bright = nullptr;

    startEvent(L"DrawScreenQuadEvalBrightness");
    if (!createRenderTargetTexture(width, height, rtv_bright, srv_bright, samplerState, false))
        return false;

    BrightnessConstantBuffer cb;
    ZeroMemory(&cb, sizeof(BrightnessConstantBuffer));

    setViewport(width, height);
    setRenderTarget(rtv_bright);
    cb.isBrightnessCalc = 1;
    brightnessCbuf->update(cb);
    screenQuadPrim->render(brightShader, { brightnessCbuf->appliedConstBuffer() }, { 0 }, samplerState, baseSRV);
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
        if (!createRenderTargetTexture(two_pow_n, two_pow_n, rtv_2n, srv_2n, samplerState, false, cpuAccessFlag ? &tex_2n : nullptr))
            return false;

        setViewport(two_pow_n, two_pow_n);
        setRenderTarget(rtv_2n);
        screenQuadPrim->render(brightShader, { brightnessCbuf->appliedConstBuffer() }, { 0 }, samplerState, curSRV);
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
    assert(fabs(arr[0] - arr[1]) < 1e-6);
    assert(fabs(arr[1] - arr[2]) < 1e-6);
    assert(fabs(arr[3] - 1) < 1e-6);
    context->Unmap(brightnessPixelTex2D, 0);

    return std::exp(arr[0]) - 1.0f;
}


void Graphics::render() {
    moveCamera();

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
    screenQuadPrim->render(tonemapShader, { tonemapCbuf->appliedConstBuffer() }, { 0 }, samplerState, baseSRV);
    endEvent();
    
    startEvent(L"DrawBrightQuad");
    cb.isBrightnessWindow = 1;
    tonemapCbuf->update(cb);
    brightQuadPrim->render(tonemapShader, { tonemapCbuf->appliedConstBuffer() }, { 0 }, samplerState, brightnessPixelSRV);
    endEvent();

    if (brightnessPixelSRV)
        brightnessPixelSRV->Release();
    if (brightnessPixelTex2D)
        brightnessPixelTex2D->Release();

    swapChain->Present(0, 0);

    // Unbind shader resource
    ID3D11ShaderResourceView* views[1] = { nullptr };
    context->PSSetShaderResources(0, 1, views);
}

void Graphics::cleanup() {
    if (swapChainRTV) swapChainRTV->Release();
    if (baseTextureRTV) baseTextureRTV->Release();

    simpleShader.cleanup();
    brightShader.cleanup();
    tonemapShader.cleanup();

    simpleCbuf->cleanup();
    brightnessCbuf->cleanup();
    tonemapCbuf->cleanup();

    quadPrim->cleanup();
    screenQuadPrim->cleanup();
    brightQuadPrim->cleanup();

    if (swapChain1) swapChain1->Release();
    if (swapChain) swapChain->Release();
    if (annotation) annotation->Release();

    if (baseSRV) baseSRV->Release();
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

    //if (baseTextureRTV) baseTextureRTV->Release();
    context->Flush();

    hr = swapChain->ResizeBuffers(1, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
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

    setViewport(width, height);

    this->width = width;
    this->height = height;
    
    return S_OK;
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
    lightIntensity[lightIndex] = 1.0f;
}

void Graphics::increaseLightIntensity(int lightIndex) {
    lightIntensity[lightIndex] = min(lightIntensity[lightIndex] + 10.0f, 10000.0f);
}

void Graphics::decreaseLightIntensity(int lightIndex) {
    lightIntensity[lightIndex] = max(lightIntensity[lightIndex] - 10.0f, 0.0f);
}
