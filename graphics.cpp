#include <optional>
#include <vector>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxcolors.h>
#include <chrono>

#include "graphics.h"


std::shared_ptr<Graphics> Graphics::init(HWND hWnd) {
    std::shared_ptr<Graphics> graphics(new Graphics);

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

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = graphics->swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
    if (FAILED(hr))
        return nullptr;

    hr = graphics->device->CreateRenderTargetView(pBackBuffer, nullptr, &graphics->renderTargetView);
    pBackBuffer->Release();
    if (FAILED(hr))
        return nullptr;

    graphics->context->OMSetRenderTargets(1, &graphics->renderTargetView, nullptr);

    // Setup the viewport
    D3D11_VIEWPORT vp;
    ZeroMemory(&vp, sizeof(D3D11_VIEWPORT));
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    graphics->context->RSSetViewports(1, &vp);

    graphics->initGeometry();

    return graphics;
}

//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DCompile
//
// With VS 11, we could load up prebuilt .cso files instead...
//--------------------------------------------------------------------------------------
HRESULT Graphics::CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut) {
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;

    // Disable optimizations to further improve shader debugging
    dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ID3DBlob* pErrorBlob = nullptr;
    hr = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel,
        dwShaderFlags, 0, ppBlobOut, &pErrorBlob);
    if (FAILED(hr)) {
        if (pErrorBlob) {
            OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
            pErrorBlob->Release();
        }
        return hr;
    }
    if (pErrorBlob) pErrorBlob->Release();

    return S_OK;
}

void Graphics::initGeometry() {
    // Compile the vertex shader
    ID3DBlob* pVSBlob = nullptr;
    auto hr = CompileShaderFromFile(L"simple.fx", "VS", "vs_4_0", &pVSBlob);
    if (FAILED(hr)) {
        MessageBox(nullptr,
            L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return;
    }

    // Create the vertex shader
    hr = device->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &vertexShader);
    if (FAILED(hr)) {
        pVSBlob->Release();
        return;
    }

    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = ARRAYSIZE(layout);

    // Create the input layout
    hr = device->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
        pVSBlob->GetBufferSize(), &vertexLayout);

    if (FAILED(hr))
        return;

    // Set the input layout
    context->IASetInputLayout(vertexLayout);

    // Compile the pixel shader
    ID3DBlob* pPSBlob = nullptr;
    hr = CompileShaderFromFile(L"simple.fx", "PS", "ps_4_0", &pPSBlob);
    if (FAILED(hr)) {
        MessageBox(nullptr,
            L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return;
    }

    // Create the pixel shader
    hr = device->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &pixelShader);
    pPSBlob->Release();

    if (FAILED(hr))
        return;

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

    // Init vertex buffer
    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(vertices);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = vertices;
    hr = device->CreateBuffer(&bd, &InitData, &vertexBuffer);
    if (FAILED(hr))
        return;

    // Set vertex buffer
    UINT stride = sizeof(SimpleVertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);

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

    // Init index buffer
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(indices);
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;

    //ZeroMemory( &InitData, sizeof(InitData) );
    InitData.pSysMem = indices;
    hr = device->CreateBuffer(&bd, &InitData, &indexBuffer);
    if (FAILED(hr))
        return;

    // Set index buffer
    context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

    // Set primitive topology
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Create the constant buffer
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(ConstantBuffer);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    hr = device->CreateBuffer(&bd, nullptr, &constBuffer);
    if (FAILED(hr))
        return;

    // Initialize the world matrix
    world = XMMatrixIdentity();

    // Initialize the view matrix
    XMVECTOR Eye = XMVectorSet(0.0f, 0.0f, -5.0f, 0.0f);
    XMVECTOR At = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    view = XMMatrixLookAtLH(Eye, At, Up);

    // Initialize the projection matrix
    projection = XMMatrixPerspectiveFovLH(XM_PIDIV2, 1, 0.001f, 100.0f);
}


void Graphics::render() {
    // Just clear the backbuffer
    float clearColor[] = {0.3f, 0.5f, 0.7f, 1.f};
    context->ClearRenderTargetView(renderTargetView, clearColor);

    //
    // Update variables
    //
    ConstantBuffer cb;
    ZeroMemory(&cb, sizeof(ConstantBuffer));

    auto time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count() / 1000.0f;

    world = XMMatrixRotationY(time);

    cb.mWorld = XMMatrixTranspose(world);
    cb.mView = XMMatrixTranspose(view);
    cb.mProjection = XMMatrixTranspose(projection);
    cb.mTranslation = XMMatrixTranslation(.0f, 0.2f, .0f);
#ifdef _DEBUG
    annotation->BeginEvent(L"UpdConstBuffer");
#endif
    context->UpdateSubresource(constBuffer, 0, nullptr, &cb, 0, 0);
#ifdef _DEBUG
    annotation->EndEvent();
#endif

    // Render a triangle
#ifdef _DEBUG
    annotation->BeginEvent(L"DrawTriangle");
#endif
    context->VSSetShader(vertexShader, nullptr, 0);
    context->VSSetConstantBuffers(0, 1, &constBuffer);
    context->PSSetShader(pixelShader, nullptr, 0);
    context->DrawIndexed(36, 0, 0);
#ifdef _DEBUG
    annotation->EndEvent();
#endif

    swapChain->Present(0, 0);
}

void Graphics::cleanup() {
    if (renderTargetView) renderTargetView->Release();
    if (vertexShader) vertexShader->Release();
    if (pixelShader) pixelShader->Release();
    if (vertexBuffer) vertexBuffer->Release();
    if (vertexLayout) vertexLayout->Release();
    if (indexBuffer) indexBuffer->Release();
    if (constBuffer) constBuffer->Release();
    if (swapChain1) swapChain1->Release();
    if (swapChain) swapChain->Release();
    if (annotation) annotation->Release();

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

    // Initialize the projection matrix
    projection = XMMatrixPerspectiveFovLH(XM_PIDIV2, width / (FLOAT)height, 0.01f, 100.0f);

    context->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
    renderTargetView->Release();
    context->Flush();

    hr = swapChain->ResizeBuffers(1, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
    if (FAILED(hr)) {
        return hr;
    }

    ID3D11Texture2D* backBuffer = nullptr;
    hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
    if (FAILED(hr)) {
        return hr;
    }

    hr = device->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView);
    if (FAILED(hr)) {
        return hr;
    }
    backBuffer->Release();

    CD3D11_VIEWPORT viewPort(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
    context->RSSetViewports(1, &viewPort);

    context->OMSetRenderTargets(1, &renderTargetView, nullptr);
    return S_OK;
}

