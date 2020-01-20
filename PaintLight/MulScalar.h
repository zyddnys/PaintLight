#pragma once

#include "DXUT.h"
#include "d3d11helper.h"
#include "RGBAImage.h"

class MulScalar
{
private:
	std::size_t const GroupThreads = 32;
	ID3D11ComputeShader *m_cs;
	ID3D11Buffer *m_buf;

	struct scalar
	{
		float valueR, valueG, valueB;
		std::uint32_t pad1;
	};
public:
	MulScalar() : m_cs(nullptr), m_buf(nullptr)
	{
		;
	}
	MulScalar(ID3D11Device *device, ID3D11DeviceContext *context)
	{
		ID3DBlob *csByteCodes{ nullptr };
		THROW(CompileShader(L"MulScalar.hlsl", nullptr, "main", "cs_5_0", std::addressof(csByteCodes)));
		THROW(device->CreateComputeShader(csByteCodes->GetBufferPointer(), csByteCodes->GetBufferSize(), nullptr, std::addressof(m_cs)));

		D3D11_BUFFER_DESC bufDesc;
		bufDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufDesc.MiscFlags = 0;
		bufDesc.ByteWidth = sizeof(scalar);

		THROW(device->CreateBuffer(std::addressof(bufDesc), nullptr, std::addressof(m_buf)));
	}

	void Release() noexcept
	{
		try {
			SAFE_RELEASE(m_cs);
			SAFE_RELEASE(m_buf);
		}
		catch (...) {

		}
	}

	~MulScalar()
	{
		Release();
	}

	MulScalar(MulScalar const &other) = delete;
	MulScalar &operator=(MulScalar const &other) = delete;

	MulScalar(MulScalar &&other) noexcept : m_cs(other.m_cs), m_buf(other.m_buf)
	{
		other.m_cs = nullptr;
		other.m_buf = nullptr;
	}
	MulScalar &operator=(MulScalar &&other) noexcept
	{
		if (std::addressof(other) != this)
		{
			Release();
			m_cs = other.m_cs;
			m_buf = other.m_buf;
			other.m_cs = nullptr;
			other.m_buf = nullptr;
		}
		return *this;
	}
public:
	void operator()(ID3D11Device *device, ID3D11DeviceContext *context, RGBAImageGPU const &input, float valueR, float valueG, float valueB, RGBAImageGPU &ans)
	{
		if (input != ans)
			throw std::runtime_error("input and output shape mismatch");

		D3D11_MAPPED_SUBRESOURCE mappedResource;

		// set scalar
		THROW(context->Map(m_buf, 0, D3D11_MAP_WRITE_DISCARD, 0, std::addressof(mappedResource)));
		auto scalarBuf = static_cast<scalar *>(mappedResource.pData);
		scalarBuf->valueR = valueR;
		scalarBuf->valueG = valueG;
		scalarBuf->valueB = valueB;
		context->Unmap(m_buf, 0);

		context->CSSetShader(m_cs, nullptr, 0);
		context->CSSetShaderResources(0, 1, std::addressof(input.srv));
		context->CSSetUnorderedAccessViews(0, 1, std::addressof(ans.uav), nullptr);
		context->CSSetConstantBuffers(0, 1, std::addressof(m_buf)); // set CB

		std::size_t groupX(((input.width - 1) / GroupThreads) + 1);
		std::size_t groupY(((input.height - 1) / GroupThreads) + 1);
		context->Dispatch(groupX, groupY, 1);

		context->CSSetShader(nullptr, nullptr, 0);
		context->CSSetUnorderedAccessViews(0, 1, std::addressof(g_nullUAV), nullptr);
		context->CSSetShaderResources(0, 1, std::addressof(g_nullSRV));
		context->CSSetConstantBuffers(0, 1, std::addressof(g_nullCB));
	}
	RGBAImageGPU operator()(ID3D11Device *device, ID3D11DeviceContext *context, RGBAImageGPU const &input, float valueR, float valueG, float valueB)
	{
		RGBAImageGPU ans(device, input); // empty_like
		this->operator()(device, context, input, valueR, valueG, valueB, ans);
		return ans;
	}
};
