#include <vector>
#include <string>
#include <d3d11_1.h>
#include <directxcolors.h>
#include <chrono>
#include <cmath>
#include <tuple>

#include "graphics.h"
#include "camera.h"
#include "shader.h"
#include "primitive.h"


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
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    graphics->simple.makeShaders(L"simple.fx", simpleLayout, 2);

    D3D11_INPUT_ELEMENT_DESC brightLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    graphics->bright.makeShaders(L"brightness.fx", brightLayout, 3);

    if (!graphics->CreateRenderTargetTexture(width, height, inst->baseTextureRTV, inst->samplerState, inst->baseSRV))
        return nullptr; 

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = graphics->swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
    if (FAILED(hr))
        return nullptr;

    hr = graphics->device->CreateRenderTargetView(pBackBuffer, nullptr, &graphics->renderTargetView);
    pBackBuffer->Release();
    if (FAILED(hr))
        return nullptr;

    //ID3D11RenderTargetView* views[] = { graphics->renderTargetView, graphics->baseTextureRTV };
    //graphics->context->OMSetRenderTargets(2, views, nullptr);
    graphics->context->OMSetRenderTargets(1, &graphics->baseTextureRTV, nullptr);
    graphics->context->OMSetRenderTargets(1, &graphics->renderTargetView, nullptr);
    graphics->setViewport(width, height);

    if (!graphics->initGeometry())
        return nullptr;

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

bool Graphics::CreateRenderTargetTexture(
    UINT width, UINT height, ID3D11RenderTargetView*& rtv,
    ID3D11SamplerState*& samplerState,
    ID3D11ShaderResourceView*& srv)
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

    // Setup the description of the render target view.
    D3D11_RENDER_TARGET_VIEW_DESC rtvd;

    rtvd.Format = td.Format;
    rtvd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    rtvd.Texture2D.MipSlice = 0;

    hr = inst->device->CreateRenderTargetView(rndTargetTexWH, &rtvd, &rtv);
    if (FAILED(hr))
        return false;

    rndTargetTexWH->Release();

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

    // Setup the description of the shader resource view.
    D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
    srvd.Format = td.Format;
    srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvd.Texture2D.MostDetailedMip = 0;
    srvd.Texture2D.MipLevels = 1;

    // Create the shader resource view.
    hr = inst->device->CreateShaderResourceView(rndTargetTexWH, &srvd, &srv);
    if (FAILED(hr))
        return false;

    return true;
}


bool Graphics::initGeometry() {
    
    // Create vertex buffer
    SimpleVertex vertices[] =
    {
        { XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
        { XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
        { XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) },
        { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f) },
        { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) },
    };

    // Create index buffer
    UINT indices[] =
    {
        3,1,0,
        2,1,3,

        0,5,4,
        1,5,0,

        3,4,7,
        0,4,3,

        1,6,5,
        2,6,1,

        2,7,6,
        3,7,2,
        
        6,4,5,
        7,4,6,
    };

    cube = PrimitiveFactory::create<SimpleVertex>(vertices, 8, indices, 36);
    if (!cube)
        return false;

    cube->addConstBuffer(sizeof(ConstantBuffer));

    // Initialize the world matrix
    world = XMMatrixIdentity();

    return true;
}


void Graphics::render() {
    // Just clear the backbuffer
    float clearColor[] = { 0.3f, 0.5f, 0.7f, 1.0f };
    context->ClearRenderTargetView(renderTargetView, clearColor);

    // Camera movement
    XMVECTOR moveDirection = { 0.0f, 0.0f, 0.0f, 0.0f };
    if (moveRight) moveDirection += camera.getRight();
    if (moveLeft) moveDirection += -camera.getRight();
    if (moveForward) moveDirection += camera.getDirection();
    if (moveBackward) moveDirection -= camera.getDirection();
    if (moveUp) moveDirection += { 0.0f, 1.0f, 0.0f, 0.0f };
    if (moveDown) moveDirection += { 0.0f, -1.0f, 0.0f, 0.0f };

    float deltaTime = (timeGetTime() - lastFrame) / 1000.0f;
    moveDirection *= moveSpeed * deltaTime;

    if (XMVectorGetX(XMVector3Length(moveDirection)) > 1e-4) {
		camera.move(moveDirection);
    }

    lastFrame = timeGetTime();

    // Update variables
    ConstantBuffer cb;
    ZeroMemory(&cb, sizeof(ConstantBuffer));

    auto time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count() / 1000.0f;

    world = XMMatrixTranslation(.0f, 0.9 * sin(time), .0f) * XMMatrixRotationY(time);

    cb.mWorld = XMMatrixTranspose(world);
    cb.mView = XMMatrixTranspose(camera.view());
    cb.mProjection = XMMatrixTranspose(camera.projection());


#ifdef _DEBUG
    annotation->BeginEvent(L"UpdConstBuffer");
#endif
    cube->updateConstBuffer<ConstantBuffer>(0, cb);
#ifdef _DEBUG
    annotation->EndEvent();
#endif

    // Render a cube
#ifdef _DEBUG
    annotation->BeginEvent(L"DrawCube");
#endif
    cube->render(simple);
#ifdef _DEBUG
    annotation->EndEvent();
#endif
    swapChain->Present(0, 0);
}

void Graphics::cleanup() {
    if (renderTargetView) renderTargetView->Release();
    simple.cleanup();
    bright.cleanup();
    cube->cleanup();
    if (swapChain1) swapChain1->Release();
    if (swapChain) swapChain->Release();
    if (annotation) annotation->Release();

    if (baseTextureRTV) baseTextureRTV->Release();
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
    /*
    HRESULT hr;
    ID3D11RenderTargetView* nullViews [] = { nullptr };

    camera.setAspectRatio(width / (FLOAT)height);

    context->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
    //renderTargetView->Release();
    context->Flush();

    hr = swapChain->ResizeBuffers(1, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
    if (FAILED(hr))
        return hr;

    ID3D11Texture2D* backBuffer = nullptr;
    hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
    if (FAILED(hr))
        return hr;

    hr = device->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView);
    if (FAILED(hr))
        return hr;
 
    backBuffer->Release();

    CD3D11_VIEWPORT viewPort(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
    context->RSSetViewports(1, &viewPort);

    context->OMSetRenderTargets(1, &renderTargetView, nullptr);
    */
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
