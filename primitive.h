#pragma once

#include <vector>
#include <d3d11_1.h>
#include <any>
#include "graphics.h"
#include "const_buffer.h"


class Primitive
{
public:
    void cleanup();

    void render(std::shared_ptr<Shader> shader,
        ID3D11SamplerState* samplerState = nullptr, ID3D11ShaderResourceView* tex = nullptr);

private:
    template<typename VertexType>
    bool create(
        VertexType const* vertices, UINT vCount, 
        UINT const* indices, UINT iCount, D3D11_PRIMITIVE_TOPOLOGY topology)
    {
        graphics = Graphics::get();
        this->iCount = iCount;
        this->topology = topology;

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
    std::shared_ptr<Graphics> graphics;
    UINT stride;
    UINT offset;
    D3D11_PRIMITIVE_TOPOLOGY topology;


    friend class PrimitiveFactory;
};


class PrimitiveFactory
{
public:
    template<typename VertexType, UINT VNum, UINT INum>
    static std::unique_ptr<Primitive> create(
        std::array<VertexType, VNum> const &vertices, std::array<UINT, INum> const& indices,
        D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
    {
        return create(vertices.data(), VNum, indices.data(), INum, topology);
    }

    template<typename VertexType>
    static std::unique_ptr<Primitive> create(
        VertexType const* vertices, UINT vCount, UINT const* indices, UINT iCount,
        D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
    {
        auto pr = std::unique_ptr<Primitive>(new Primitive);
        if (!pr->create(vertices, vCount, indices, iCount, topology))
            return nullptr;
        return pr;
    }
};
