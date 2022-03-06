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

void Primitive::render(Shader const& shader, ID3D11SamplerState* samplerState, ID3D11ShaderResourceView* tex)
{
    shader.apply();
    auto ctx = graphics->getContext();
    ctx->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    ctx->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

    // Set primitive topology
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetConstantBuffers(0, static_cast<UINT>(constBuffers.size()), constBuffers.data());
    if (tex && samplerState)
    {
        // Set the sampler state in the pixel shader.
        ctx->PSSetSamplers(0, 1, &samplerState);
        ctx->PSSetShaderResources(0, 1, &tex);
    }
    ctx->DrawIndexed(iCount, 0, 0);
}
