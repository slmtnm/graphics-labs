#pragma once

#include <d3d11_1.h>
#include <memory>

class Graphics;

class Shader
{
public:
	Shader() = default;

	ID3D11VertexShader* vertexShader() const;
	ID3D11PixelShader* pixelShader() const;
	ID3D11InputLayout* vertexLayout() const;

	void makeShaders(LPCWSTR shaderName, D3D11_INPUT_ELEMENT_DESC* layout, int numElementsLayout);

	void cleanup();

private:
	static HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);

	ID3D11VertexShader* _vertexShader;
	ID3D11PixelShader* _pixelShader;
	ID3D11InputLayout* _vertexLayout;
};

