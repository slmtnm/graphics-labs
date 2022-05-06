#pragma once

#include <memory>
#include <directxmath.h>

using namespace DirectX;

class Primitive;

struct SimpleVertex
{
    XMFLOAT3 Pos;
    XMFLOAT3 Norm;
    XMFLOAT4 Color;
};

struct TextureVertex
{
    XMFLOAT3 Pos;
    XMFLOAT4 Color;
    XMFLOAT2 Tex;
};


namespace PrimitiveSample
{
    bool createQuad(std::shared_ptr<Primitive>& prim);
    bool createSphere(std::shared_ptr<Primitive>& prim, float R, bool invDir = false);
    bool createScreenQuad(std::shared_ptr<Primitive>& prim, bool full, float val = 0.0f);
    bool createQuad(std::shared_ptr<Primitive>& prim);
};
