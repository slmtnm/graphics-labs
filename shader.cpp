#include <d3dcompiler.h>

#include "graphics.h"
#include "Shader.h"


//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DCompile
//
// With VS 11, we could load up prebuilt .cso files instead...
//--------------------------------------------------------------------------------------
HRESULT Shader::CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut) {
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

void Shader::makeShaders(LPCWSTR shaderName, D3D11_INPUT_ELEMENT_DESC* layout, int numElementsLayout) {
    // Compile the vertex shader
    ID3DBlob* pVSBlob = nullptr;
    auto hr = CompileShaderFromFile(shaderName, "VS", "vs_4_0", &pVSBlob);
    if (FAILED(hr)) {
        MessageBox(nullptr,
            L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return;
    }

    // Create the vertex shader
    auto graphics = Graphics::get();
    hr = graphics->getDevice()->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &_vertexShader);
    if (FAILED(hr)) {
        pVSBlob->Release();
        return;
    }

    // Create the input layout
    hr = graphics->getDevice()->CreateInputLayout(
        layout, numElementsLayout, pVSBlob->GetBufferPointer(),
        pVSBlob->GetBufferSize(), &_vertexLayout);

    if (FAILED(hr))
        return;

    // Compile the pixel shader
    ID3DBlob* pPSBlob = nullptr;
    hr = CompileShaderFromFile(shaderName, "PS", "ps_4_0", &pPSBlob);
    if (FAILED(hr)) {
        MessageBox(nullptr,
            L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return;
    }

    // Create the pixel shader
    hr = graphics->getDevice()->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &_pixelShader);
    pPSBlob->Release();

    if (FAILED(hr))
        return;
}

ID3D11VertexShader* Shader::vertexShader() const
{
    return _vertexShader;
}

ID3D11PixelShader* Shader::pixelShader() const
{
    return _pixelShader;
}

ID3D11InputLayout* Shader::vertexLayout() const
{
    return _vertexLayout;
}

void Shader::cleanup()
{
    if (_vertexShader)
        _vertexShader->Release();

    if (_pixelShader)
        _pixelShader->Release();

    if (_vertexLayout)
        _vertexLayout->Release();
}
