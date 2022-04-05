#pragma once

#include <memory>
#include <windows.h>
#include "graphics.h"

class AppliedConstBuffer;

template<typename ConstBufferType>
class ConstBuffer
{
public:
    ConstBuffer& operator=(ConstBuffer const&) = delete;
    ConstBuffer(ConstBuffer const&) = delete;

    std::shared_ptr<AppliedConstBuffer> appliedConstBuffer() const
    {
        return std::shared_ptr<AppliedConstBuffer>(new AppliedConstBuffer(constBuffer, status));
    }

    void update(ConstBufferType const& cb)
    {
        if (status)
        {
            auto graphics = Graphics::get();
            graphics->getContext()->UpdateSubresource(constBuffer, 0, nullptr, &cb, 0, 0);
        }
    }

    void cleanup()
    {
        constBuffer->Release();
    }

    ConstBuffer()
    {
        D3D11_BUFFER_DESC bd;
        ZeroMemory(&bd, sizeof(bd));

        // Create the constant buffer
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = sizeof(ConstBufferType);
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = 0;

        auto graphics = Graphics::get();
        auto hr = graphics->getDevice()->CreateBuffer(&bd, nullptr, &constBuffer);

        status = SUCCEEDED(hr);
    }

private:
	ID3D11Buffer* constBuffer;
	bool status = false;
};

class AppliedConstBuffer
{
public:
    void apply(UINT cbufRegister, bool boundVS, bool boundPS) const;

private:
    AppliedConstBuffer(ID3D11Buffer* buffer, bool status) : 
        constBuffer(buffer), status(status) {}

    ID3D11Buffer* constBuffer;
    bool status = false;

    template<typename T>
    friend class ConstBuffer;
};
