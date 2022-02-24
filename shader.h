#pragma once

#include <d3d11_1.h>
#include <memory>

class Graphics;

class Shader
{
public:
	Shader() = default;
	Shader(std::shared_ptr<Graphics> graphics) : graphics(graphics) {}
	void SetGraphicsPtr(std::shared_ptr<Graphics> ngraphics);

	ID3D11VertexShader* vertexShader() const;
	ID3D11PixelShader* pixelShader() const;

	void MakeShaders(LPCWSTR shader_name, D3D11_INPUT_ELEMENT_DESC* layout, int numElementsLayout);

	void cleanup();

private:
	static HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);

	std::shared_ptr<Graphics> graphics;

	ID3D11VertexShader* _vertexShader;
	ID3D11PixelShader* _pixelShader;
};

