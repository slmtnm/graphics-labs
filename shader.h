#pragma once

#include <d3d11_1.h>
#include <memory>

class Graphics;
class AppliedConstBuffer;

class Shader
{
public:
	struct ConstBufferData
	{
		std::shared_ptr<AppliedConstBuffer> constBuffer;
		bool bindVS;
		bool bindPS;
	};

	void addConstBuffers(std::vector<ConstBufferData> const& constBuffers);
	void apply() const;

	ID3D11VertexShader* vertexShader() const;
	ID3D11PixelShader* pixelShader() const;
	ID3D11InputLayout* vertexLayout() const;

	void cleanup();
	std::vector<ConstBufferData> const& getConstBuffers() const;

private:
	Shader() = default;

	void makeShaders(LPCWSTR shaderName, D3D11_INPUT_ELEMENT_DESC* layout, int numElementsLayout);
	static HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);

	ID3D11VertexShader* _vertexShader;
	ID3D11PixelShader* _pixelShader;
	ID3D11InputLayout* _vertexLayout;

	std::vector<ConstBufferData> constBuffers;
	bool status = false;

	friend class ShaderFactory;
};

class ShaderFactory
{
public:
	static std::unique_ptr<Shader> makeShaders(LPCWSTR shaderName, D3D11_INPUT_ELEMENT_DESC* layout, int numElementsLayout);
};

