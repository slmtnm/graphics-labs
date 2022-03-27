#pragma once

#include <vector>
#include <d3d11_1.h>
#include <any>
#include "graphics.h"


class Primitive
{
public:
    UINT addConstBuffer(UINT cBufSize, bool boundVS, bool boundPS);

    void cleanup();

    template<typename ConstBuffer>
    void updateConstBuffer(UINT cbufIdx, ConstBuffer const& cb)
    {
        auto graphics = Graphics::get();
        graphics->getContext()->UpdateSubresource(constBuffers[cbufIdx], cbufIdx, nullptr, &cb, 0, 0);
    }

    void render(Shader const& shader, ID3D11SamplerState* samplerState = nullptr, ID3D11ShaderResourceView* tex = nullptr);

private:
    template<typename VertexType>
    bool create(VertexType* vertices, UINT vCount, UINT* indices, UINT iCount)
    {
        graphics = Graphics::get();
        this->iCount = iCount;

        // Init vertex buffer
        D3D11_BUFFER_DESC bd;
        ZeroMemory(&bd, sizeof(bd));
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = sizeof(VertexType) * vCount;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = 0;
        D3D11_SUBRESOURCE_DATA InitData;
        ZeroMemory(&InitData, sizeof(InitData));
        InitData.pSysMem = vertices;
        auto hr = graphics->getDevice()->CreateBuffer(&bd, &InitData, &vertexBuffer);
        if (FAILED(hr))
            return false;

        // Set vertex buffer
        stride = sizeof(VertexType);
        offset = 0;

        // Create index buffer
        // Init index buffer
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = sizeof(UINT) * iCount;
        bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        bd.CPUAccessFlags = 0;

        //ZeroMemory( &InitData, sizeof(InitData) );
        InitData.pSysMem = indices;
        hr = graphics->getDevice()->CreateBuffer(&bd, &InitData, &indexBuffer);
        if (FAILED(hr)) {
            return false;
        }

        return true;
    }

    Primitive() = default;
    Primitive(Primitive const&) = delete;
    Primitive & operator=(Primitive const&) = delete;

    UINT iCount;
    ID3D11Buffer* vertexBuffer = nullptr;
    ID3D11Buffer* indexBuffer = nullptr;
    std::vector<ID3D11Buffer*> constBuffers;
    std::shared_ptr<Graphics> graphics;
    UINT stride;
    UINT offset;

    bool boundVS;
    bool boundPS;

    friend class PrimitiveFactory;
};


class PrimitiveFactory
{
public:
    template<typename VertexType>
    static std::unique_ptr<Primitive> create(VertexType* vertices, UINT vCount, UINT* indices, UINT iCount)
    {
        auto pr = std::unique_ptr<Primitive>(new Primitive);
        if (!pr->create(vertices, vCount, indices, iCount))
            return nullptr;
        return pr;
    }
};
