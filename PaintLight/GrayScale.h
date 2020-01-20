#pragma once

#include "DXUT.h"
#include "d3d11helper.h"
#include "RGBAImage.h"

class GrayScale
{
private:
	std::size_t const GroupThreads = 32;
	ID3D11ComputeShader *m_cs;
public:
	GrayScale(): m_cs(nullptr)
	{
		;
	}
	GrayScale(ID3D11Device *device, ID3D11DeviceContext *context)
	{
		ID3DBlob *csByteCodes{ nullptr };
		THROW(CompileShader(L"GrayScale.hlsl", nullptr, "main", "cs_5_0", std::addressof(csByteCodes)));
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

	~GrayScale()
	{
		Release();
	}

	GrayScale(GrayScale const &other) = delete;
	GrayScale &operator=(GrayScale const &other) = delete;

	GrayScale(GrayScale &&other) noexcept: m_cs(other.m_cs)
	{
		other.m_cs = nullptr;
	}
	GrayScale &operator=(GrayScale &&other) noexcept
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
	void operator()(ID3D11Device *device, ID3D11DeviceContext *context, RGBAImageGPU const &input, RGBAImageGPU &ans)
	{
		if (input != ans)
			throw std::runtime_error("input and output shape mismatch");

		context->CSSetShader(m_cs, nullptr, 0);
		context->CSSetShaderResources(0, 1, std::addressof(input.srv));
		context->CSSetUnorderedAccessViews(0, 1, std::addressof(ans.uav), nullptr);

		std::size_t groupX(((input.width - 1) / GroupThreads) + 1);
		std::size_t groupY(((input.height - 1) / GroupThreads) + 1);
		context->Dispatch(groupX, groupY, 1);

		context->CSSetShader(nullptr, nullptr, 0);
		context->CSSetUnorderedAccessViews(0, 1, std::addressof(g_nullUAV), nullptr);
		context->CSSetShaderResources(0, 1, std::addressof(g_nullSRV));
	}
	RGBAImageGPU operator()(ID3D11Device *device, ID3D11DeviceContext *context, RGBAImageGPU const &input)
	{
		RGBAImageGPU ans(device, input); // empty_like
		this->operator()(device, context, input, ans);
		return ans;
	}
};
