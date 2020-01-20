#pragma once

#include "DXUT.h"
#include "d3d11helper.h"
#include "RGBAImage.h"
#include "MulScalar.h"
#include "ImageMinMax.h"

class Lighting
{
private:
	std::size_t const GroupThreads = 32;
	ID3D11ComputeShader *m_cs;
	ID3D11ComputeShader *m_csSobel;
	ID3D11Buffer *m_buf;

	struct LightingInfo
	{
		std::uint32_t width, height;
		float light_source_x, light_source_y, light_source_z;
		float pixel_scale;
		float delta_pd;
		std::uint32_t pad1;
	};

	MulScalar m_mulScalar;
	ImageMinMax<> m_imageMinMax;
public:
	Lighting() : m_cs(nullptr), m_csSobel(nullptr), m_buf(nullptr)
	{
		;
	}
	Lighting(ID3D11Device *device, ID3D11DeviceContext *context)
	{
		ID3DBlob *csByteCodes{ nullptr };
		THROW(CompileShader(L"Lighting.hlsl", nullptr, "main", "cs_5_0", std::addressof(csByteCodes)));
		THROW(device->CreateComputeShader(csByteCodes->GetBufferPointer(), csByteCodes->GetBufferSize(), nullptr, std::addressof(m_cs)));

		ID3DBlob *csByteCodesSobel{ nullptr };
		THROW(CompileShader(L"Lighting.hlsl", nullptr, "sobel", "cs_5_0", std::addressof(csByteCodesSobel)));
		THROW(device->CreateComputeShader(csByteCodesSobel->GetBufferPointer(), csByteCodesSobel->GetBufferSize(), nullptr, std::addressof(m_csSobel)));

		D3D11_BUFFER_DESC bufDesc;
		bufDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufDesc.MiscFlags = 0;
		bufDesc.ByteWidth = sizeof(LightingInfo);

		THROW(device->CreateBuffer(std::addressof(bufDesc), nullptr, std::addressof(m_buf)));

		m_mulScalar = MulScalar(device, context);
		m_imageMinMax = ImageMinMax(device, context);
	}

	void Release() noexcept
	{
		try {
			SAFE_RELEASE(m_cs);
			SAFE_RELEASE(m_csSobel);
			SAFE_RELEASE(m_buf);
			m_mulScalar.Release();
			m_imageMinMax.Release();
		}
		catch (...) {

		}
	}

	~Lighting()
	{
		Release();
	}

	Lighting(Lighting const &other) = delete;
	Lighting &operator=(Lighting const &other) = delete;

	Lighting(Lighting &&other) noexcept :
		m_cs(other.m_cs),
		m_csSobel(other.m_csSobel),
		m_buf(other.m_buf),
		m_mulScalar(std::move(other.m_mulScalar)),
		m_imageMinMax(std::move(other.m_imageMinMax))
	{
		other.m_cs = nullptr;
		other.m_csSobel = nullptr;
		other.m_buf = nullptr;
	}
	Lighting &operator=(Lighting &&other) noexcept
	{
		if (std::addressof(other) != this)
		{
			Release();
			m_cs = other.m_cs;
			m_buf = other.m_buf;
			m_csSobel = other.m_csSobel;
			m_mulScalar = std::move(other.m_mulScalar);
			m_imageMinMax = std::move(other.m_imageMinMax);
			other.m_cs = nullptr;
			other.m_buf = nullptr;
			other.m_csSobel = nullptr;
		}
		return *this;
	}
public:
	void operator()(
		ID3D11Device *device,
		ID3D11DeviceContext *context,
		RGBAImageGPU const &input,
		RGBAImageGPU const &strokeDensity,
		float light_source_x,
		float light_source_y,
		float light_source_z,
		float pixel_scale,
		float delta_pd,
		RGBAImageGPU &ans
		)
	{
		if (input != ans)
			throw std::runtime_error("input and output shape mismatch");

		D3D11_MAPPED_SUBRESOURCE mappedResource;

		std::size_t const groupX(((input.width - 1) / GroupThreads) + 1);
		std::size_t const groupY(((input.height - 1) / GroupThreads) + 1);

		// set LightingInfo
		THROW(context->Map(m_buf, 0, D3D11_MAP_WRITE_DISCARD, 0, std::addressof(mappedResource)));
		auto LightingInfoBuf = static_cast<LightingInfo *>(mappedResource.pData);
		LightingInfoBuf->width = input.width;
		LightingInfoBuf->height = input.height;
		LightingInfoBuf->light_source_x = light_source_x;
		LightingInfoBuf->light_source_y = light_source_y;
		LightingInfoBuf->light_source_z = light_source_z;
		LightingInfoBuf->pixel_scale = pixel_scale;
		LightingInfoBuf->delta_pd = delta_pd;
		context->Unmap(m_buf, 0);


		RGBAImageGPU sobelX(device, input);
		RGBAImageGPU sobelY(device, input);
		RGBAImageGPU sobel(device, input);
		RGBAImageGPU sobelXnormalized(device, input);
		RGBAImageGPU sobelYnormalized(device, input);

		ID3D11UnorderedAccessView *uavs[] = { sobelX.uav, sobelY.uav, sobel.uav, sobelXnormalized.uav, sobelYnormalized.uav, ans.uav };
		ID3D11UnorderedAccessView *uavs_null[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
		context->CSSetShaderResources(0, 1, std::addressof(input.srv));
		context->CSSetUnorderedAccessViews(0, 6, uavs, nullptr);
		context->CSSetConstantBuffers(0, 1, std::addressof(m_buf)); // set CB

		// run sobel
		context->CSSetShader(m_csSobel, nullptr, 0);
		context->Dispatch(groupX, groupY, 1);

		context->CSSetShader(nullptr, nullptr, 0);
		context->CSSetUnorderedAccessViews(0, 6, uavs_null, nullptr);
		context->CSSetShaderResources(0, 1, std::addressof(g_nullSRV));
		context->CSSetConstantBuffers(0, 1, std::addressof(g_nullCB));

		// normalize sobel result
		auto ret(m_imageMinMax(device, context, sobel));
		auto [maxR, maxG, maxB] = std::get<1>(ret);
		m_mulScalar(device, context, sobelX, 1.0f / (maxR + 1e-10f), 1.0f / (maxG + 1e-10f), 1.0f / (maxB + 1e-10f), sobelXnormalized);
		m_mulScalar(device, context, sobelY, 1.0f / (maxR + 1e-10f), 1.0f / (maxG + 1e-10f), 1.0f / (maxB + 1e-10f), sobelYnormalized);

		context->CSSetShaderResources(1, 1, std::addressof(strokeDensity.srv));
		context->CSSetUnorderedAccessViews(0, 6, uavs, nullptr);
		context->CSSetConstantBuffers(0, 1, std::addressof(m_buf)); // set CB

		// generate lighting effect
		context->CSSetShader(m_cs, nullptr, 0);
		context->Dispatch(groupX, groupY, 1);

		context->CSSetShader(nullptr, nullptr, 0);
		context->CSSetUnorderedAccessViews(0, 6, uavs_null, nullptr);
		context->CSSetShaderResources(1, 1, std::addressof(g_nullSRV));
		context->CSSetConstantBuffers(0, 1, std::addressof(g_nullCB));
	}
	RGBAImageGPU operator()(
		ID3D11Device *device,
		ID3D11DeviceContext *context,
		RGBAImageGPU const &input,
		RGBAImageGPU const &strokeDensity,
		float light_source_x,
		float light_source_y,
		float light_source_z,
		float pixel_scale,
		float delta_pd
		)
	{
		RGBAImageGPU ans(device, input); // empty_like
		DXUT_SetDebugName(ans.srv, "Refined Lighting SRV");
		DXUT_SetDebugName(ans.uav, "Refined Lighting UAV");
		this->operator()(
			device,
			context,
			input,
			strokeDensity,
			light_source_x,
			light_source_y,
			light_source_z,
			pixel_scale,
			delta_pd,
			ans
			);
		return ans;
	}
};
