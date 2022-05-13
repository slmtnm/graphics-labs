#pragma once

#include <memory>
#include <directxmath.h>

#include "primitive.h"

using namespace DirectX;

struct SimpleVertex
{
    XMFLOAT3 Pos;
    XMFLOAT3 Norm;
    XMFLOAT4 Color;
};

struct TextureVertex
{
    XMFLOAT3 Pos;
    XMFLOAT2 Tex;
};


namespace PrimitiveSample
{
    bool createQuad(std::shared_ptr<Primitive>& prim);
    bool createSphere(std::shared_ptr<Primitive>& prim, float R, bool invDir = false, bool needTex = false);
    bool createScreenQuad(std::shared_ptr<Primitive>& prim, bool full, float val = 0.0f);
    bool createQuad(std::shared_ptr<Primitive>& prim);
    bool createCube(std::shared_ptr<Primitive>& prim, bool invDir = false);

    template<typename VertexType>
    bool createQuad(std::shared_ptr<Primitive>& prim, std::array<VertexType, 4> const& vertices)
    {       
        // Create index buffer
        std::array<UINT, 6> indices =
        {
            0, 2, 1,
            2, 0, 3
        };

        prim = PrimitiveFactory::create<VertexType>(vertices, indices);
        if (!prim)
            return false;
        return true;
    }
};
