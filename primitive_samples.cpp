#include <array>
#include <cmath>
#include <windows.h>
#include "primitive_samples.h"

#include "primitive.h"


bool PrimitiveSample::createSphere(std::shared_ptr<Primitive>& prim, float R, bool invDir, bool needTex)
{
    const int N = 50, M = 50;
    const float PI = 3.14159265359f;

    std::array<SimpleVertex, M * N> vertices;
    // TODO NEED HUGE REFACTORING. CRINGE!!!
    std::array<TextureNormVertex, M * N> tvertices;
    std::array<UINT, (N * 2 + 1) * (M - 1)> indices;

    // vertices
    for (int idx = 0, i = 0; i < N; i++)
    {
        float theta = invDir ? PI - i * PI / (N - 1) : i * PI / (N - 1);

        for (int j = 0; j < M; idx++, j++)
        {
            float phi = j * 2 * PI / (M - 1);

            float
                x = sinf(theta) * sinf(phi),
                y = cosf(theta),
                z = sinf(theta) * cosf(phi);

            if (!needTex)
            {
                vertices[idx].Pos.x = x * R;
                vertices[idx].Pos.y = y * R;
                vertices[idx].Pos.z = z * R;

                vertices[idx].Norm = XMFLOAT3(x, y, z);
                vertices[idx].Color = XMFLOAT4(1.0f, 0, 0, 1.0f);
            }
            else
            {
                tvertices[idx].Pos.x = x * R;
                tvertices[idx].Pos.y = y * R;
                tvertices[idx].Pos.z = z * R;

                tvertices[idx].Norm = XMFLOAT3(x, y, z);
                tvertices[idx].Tex = XMFLOAT2(phi / (2 * PI), 1 - (asin(y) / PI - 0.5f));
                tvertices[idx].Color = XMFLOAT4(1.0f, 0, 0, 1.0f);
            }
        }
    }

    // indices
    for (int i = 0; i < M - 1; i++)
    {
        for (int j = 0; j < N; j++)
        {
            indices[i * (N * 2 + 1) + 2 * j + 1] = (i + 1) * N + j;
            indices[i * (N * 2 + 1) + 2 * j] = i * N + j;
        }
        indices[(i + 1) * (N * 2 + 1) - 1] = -1;
    }

    if (!needTex)
        prim = PrimitiveFactory::create<SimpleVertex>(vertices, indices, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    else
        prim = PrimitiveFactory::create<TextureNormVertex>(tvertices, indices, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    if (!prim)
        return false;
    return true;
}


bool PrimitiveSample::createScreenQuad(std::shared_ptr<Primitive>& prim, bool full, float val) {
    // Create vertex buffer
    TextureVertex vertices[] =
    {
        { XMFLOAT3(-1.0f, full ? -1.0f : val, 0.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 1.0f)},
        { XMFLOAT3(full ? 1.0f : -val, full ? -1.0f : val, 0.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 1.0f)},
        { XMFLOAT3(full ? 1.0f : -val, 1.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 0.0f)},
        { XMFLOAT3(-1.0f, 1.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 0.0f)},
    };

    // Create index buffer
    UINT indices[] =
    {
        0, 2, 1,
        2, 0, 3
    };

    prim = PrimitiveFactory::create<TextureVertex>(vertices, 4, indices, 6);
    if (!prim)
        return false;
    return true;
}


bool PrimitiveSample::createQuad(std::shared_ptr<Primitive>& prim)
{
    // Create vertex buffer
    auto color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
    SimpleVertex vertices[] =
    {
        { XMFLOAT3(-5.0f, -5.0f, 10.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), color},
        { XMFLOAT3(5.0f, -5.0f, 10.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), color},
        { XMFLOAT3(5.0f, 5.0f, 10.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), color},
        { XMFLOAT3(-5.0f, 5.0f, 10.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), color},
    };

    // Create index buffer
    UINT indices[] =
    {
        0, 2, 1,
        2, 0, 3
    };

    prim = PrimitiveFactory::create<SimpleVertex>(vertices, 4, indices, 6);
    if (!prim)
        return false;
    return true;
}
