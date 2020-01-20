#pragma once

#include "DXUT.h"
#include "d3d11helper.h"
#include "RGBAImage.h"

class MulImage
{
private:
	std::size_t const GroupThreads = 32;
	ID3D11ComputeShader *m_cs;

public:
	MulImage() : m_cs(nullptr)
	{
		;
	}
	MulImage(ID3D11Device *device, ID3D11DeviceContext *context)
	{
		ID3DBlob *csByteCodes{ nullptr };
		THROW(CompileShader(L"MulImage.hlsl", nullptr, "main", "cs_5_0", std::addressof(csByteCodes)));
		THROW(device->CreateComputeShader(csByteCodes->GetBufferPointer(), csByteCodes->GetBufferSize(), nullptr, std::addressof(m_cs)));
	}

	void Release() noexcept
	{
		try {
			SAFE_RELEASE(m_cs);
		}
		catch (...) {

		}
	}

	~MulImage()
	{
		Release();
	}

	MulImage(MulImage const &other) = delete;
	MulImage &operator=(MulImage const &other) = delete;

	MulImage(MulImage &&other) noexcept : m_cs(other.m_cs)
	{
		other.m_cs = nullptr;
	}
	MulImage &operator=(MulImage &&other) noexcept
	{
		if (std::addressof(other) != this)
		{
			Release();
			m_cs = other.m_cs;
			other.m_cs = nullptr;
		}
		return *this;
	}
public:
	void operator()(ID3D11Device *device, ID3D11DeviceContext *context, RGBAImageGPU const &input, RGBAImageGPU const &input2, RGBAImageGPU &ans)
	{
		if (input != input2)
			throw std::runtime_error("two inputs shape mismatch");

		if (input != ans)
			throw std::runtime_error("input and output shape mismatch");

		D3D11_MAPPED_SUBRESOURCE mappedResource;

		context->CSSetShader(m_cs, nullptr, 0);
		context->CSSetShaderResources(0, 1, std::addressof(input.srv));
		context->CSSetShaderResources(1, 1, std::addressof(input2.srv));
		context->CSSetUnorderedAccessViews(0, 1, std::addressof(ans.uav), nullptr);

		std::size_t groupX(((input.width - 1) / GroupThreads) + 1);
		std::size_t groupY(((input.height - 1) / GroupThreads) + 1);
		context->Dispatch(groupX, groupY, 1);

		context->CSSetShader(nullptr, nullptr, 0);
		context->CSSetUnorderedAccessViews(0, 1, std::addressof(g_nullUAV), nullptr);
		context->CSSetShaderResources(0, 1, std::addressof(g_nullSRV));
		context->CSSetShaderResources(1, 1, std::addressof(g_nullSRV));
	}
	RGBAImageGPU operator()(ID3D11Device *device, ID3D11DeviceContext *context, RGBAImageGPU const &input, RGBAImageGPU const &input2)
	{
		RGBAImageGPU ans(device, input); // empty_like
		this->operator()(device, context, input, input2, ans);
		return ans;
	}
};
