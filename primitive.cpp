#include "primitive.h"

UINT Primitive::addConstBuffer(UINT cBufSize)
{
    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));

    // Create the constant buffer
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = cBufSize;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    constBuffers.push_back(nullptr);
    auto hr = graphics->getDevice()->CreateBuffer(&bd, nullptr, &constBuffers.back());
    if (FAILED(hr)) {
        return -1;
    }
    return static_cast<UINT>(constBuffers.size() - 1);
}

void Primitive::cleanup()
{
    if (vertexBuffer) vertexBuffer->Release();
    if (indexBuffer) indexBuffer->Release();

    for (auto& buf : constBuffers)
        if (buf) buf->Release();
}

void Primitive::render(Shader const& shader)
{
    shader.apply();
    graphics->getContext()->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    graphics->getContext()->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

    // Set primitive topology
    graphics->getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    graphics->getContext()->VSSetConstantBuffers(0, static_cast<UINT>(constBuffers.size()), &constBuffers[0]);
    graphics->getContext()->DrawIndexed(iCount, 0, 0);
}
