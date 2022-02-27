#pragma once

#include <vector>
#include <d3d11_1.h>
#include <any>
#include "graphics.h"


class Primitive
{
public:
    UINT addConstBuffer(UINT cBufSize);

    void cleanup();

    template<typename ConstBuffer>
    void updateConstBuffer(UINT cbufIdx, ConstBuffer const& cb)
    {
        auto graphics = Graphics::get();
        graphics->getContext()->UpdateSubresource(constBuffers[cbufIdx], cbufIdx, nullptr, &cb, 0, 0);
    }

    void render(Shader const& shader);

private:
    template<typename VertexType>
    bool create(VertexType* vertices, UINT vCount, UINT* indices, UINT iCount, Shader const& shader)
    {
        graphics = Graphics::get();
        this->iCount = iCount;

        // Set the input layout
        graphics->getContext()->IASetInputLayout(shader.vertexLayout());

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
        UINT stride = sizeof(VertexType);
        UINT offset = 0;
        graphics->getContext()->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);

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

        // Set index buffer
        graphics->getContext()->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

        // Set primitive topology
        graphics->getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
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

    friend class PrimitiveFactory;
};


class PrimitiveFactory
{
public:
    template<typename VertexType>
    static std::unique_ptr<Primitive> create(VertexType* vertices, UINT vCount, UINT* indices, UINT iCount, Shader const& shader)
    {
        auto pr = std::unique_ptr<Primitive>(new Primitive);
        if (!pr->create(vertices, vCount, indices, iCount, shader))
            return nullptr;
        return pr;
    }
};
