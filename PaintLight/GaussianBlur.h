#pragma once

#include "DXUT.h"
#include "d3d11helper.h"
#include "RGBAImage.h"

#include <cmath>

template<std::uint32_t max_radius = 92>
class GaussianBlur
{
private:
	std::size_t const GroupThreads = 256;
	ID3D11ComputeShader *m_csH;
	ID3D11ComputeShader *m_csV;
	ID3D11Buffer *m_gaussianBlurInfoBuf;
	ID3D11Buffer *m_kernelBuffer;
	ID3D11ShaderResourceView *m_kernelBufferSRV;
	float *m_kernel;
	struct gaussianBlurInfo
	{
		std::int32_t radius;
		std::int32_t width, height;
		std::int32_t pad1;
	};
public:
	GaussianBlur() : m_csH(nullptr), m_csV(nullptr), m_gaussianBlurInfoBuf(nullptr), m_kernelBuffer(nullptr), m_kernelBufferSRV(nullptr), m_kernel(nullptr)
	{
		;
	}
	GaussianBlur(ID3D11Device *device, ID3D11DeviceContext *context)
	{
		ID3DBlob *csByteCodesH{ nullptr };
		THROW(CompileShader(L"GaussianBlur.hlsl", nullptr, "mainH", "cs_5_0", std::addressof(csByteCodesH)));
		THROW(device->CreateComputeShader(csByteCodesH->GetBufferPointer(), csByteCodesH->GetBufferSize(), nullptr, std::addressof(m_csH)));

		ID3DBlob *csByteCodesV{ nullptr };
		THROW(CompileShader(L"GaussianBlur.hlsl", nullptr, "mainV", "cs_5_0", std::addressof(csByteCodesV)));
		THROW(device->CreateComputeShader(csByteCodesV->GetBufferPointer(), csByteCodesV->GetBufferSize(), nullptr, std::addressof(m_csV)));

		// create buffer for gaussianBlurInfo
		D3D11_BUFFER_DESC bufDesc;
		bufDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufDesc.MiscFlags = 0;
		bufDesc.ByteWidth = sizeof(gaussianBlurInfo);

		THROW(device->CreateBuffer(std::addressof(bufDesc), nullptr, std::addressof(m_gaussianBlurInfoBuf)));

		// create GPU buffer
		D3D11_BUFFER_DESC descBuf{};
		descBuf.Usage = D3D11_USAGE_DYNAMIC;
		descBuf.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		descBuf.ByteWidth = sizeof(float) * (max_radius * 2 + 1);
		descBuf.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		descBuf.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		descBuf.StructureByteStride = sizeof(float);

		THROW(device->CreateBuffer(std::addressof(descBuf), nullptr, std::addressof(m_kernelBuffer)));

		// create SRV
		D3D11_SHADER_RESOURCE_VIEW_DESC descSRV = {};
		descSRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
		descSRV.BufferEx.FirstElement = 0;
		descSRV.Format = DXGI_FORMAT_UNKNOWN;
		descSRV.BufferEx.NumElements = descBuf.ByteWidth / descBuf.StructureByteStride;
		THROW(device->CreateShaderResourceView(m_kernelBuffer, std::addressof(descSRV), std::addressof(m_kernelBufferSRV)));

		m_kernel = new float[max_radius * 2 + 1];
	}

	void Release() noexcept
	{
		try {
			SAFE_RELEASE(m_csH);
			SAFE_RELEASE(m_csV);
			SAFE_RELEASE(m_gaussianBlurInfoBuf);
			SAFE_RELEASE(m_kernelBuffer);
			SAFE_RELEASE(m_kernelBufferSRV);
			if (m_kernel) {
				delete[] m_kernel;
				m_kernel = nullptr;
			}
		}
		catch (...) {

		}
	}

	~GaussianBlur()
	{
		Release();
	}

	GaussianBlur(GaussianBlur const &other) = delete;
	GaussianBlur &operator=(GaussianBlur const &other) = delete;

	GaussianBlur(GaussianBlur &&other) noexcept :
		m_csH(other.m_csH),
		m_csV(other.m_csV),
		m_gaussianBlurInfoBuf(other.m_gaussianBlurInfoBuf),
		m_kernelBuffer(other.m_kernelBuffer),
		m_kernelBufferSRV(other.m_kernelBufferSRV),
		m_kernel(other.m_kernel)
	{
		other.m_csH = nullptr;
		other.m_csV = nullptr;
		other.m_gaussianBlurInfoBuf = nullptr;
		other.m_kernelBuffer = nullptr;
		other.m_kernelBufferSRV = nullptr;
		other.m_kernel = nullptr;
	}
	GaussianBlur &operator=(GaussianBlur &&other) noexcept
	{
		if (std::addressof(other) != this)
		{
			Release();
			m_csH = other.m_csH;
			m_csV = other.m_csV;
			m_gaussianBlurInfoBuf = other.m_gaussianBlurInfoBuf;
			m_kernelBuffer = other.m_kernelBuffer;
			m_kernelBufferSRV = other.m_kernelBufferSRV;
			m_kernel = other.m_kernel;
			other.m_csH = nullptr;
			other.m_csV = nullptr;
			other.m_gaussianBlurInfoBuf = nullptr;
			other.m_kernelBuffer = nullptr;
			other.m_kernelBufferSRV = nullptr;
			other.m_kernel = nullptr;
		}
		return *this;
	}
public:
	void operator()(ID3D11Device *device, ID3D11DeviceContext *context, RGBAImageGPU const &input, std::uint32_t radius, double sigma, RGBAImageGPU &ans)
	{
		if (input != ans)
			throw std::runtime_error("input and output shape mismatch");

		if (radius > max_radius)
			throw std::runtime_error("radius too high");

		// create gaussian blur kernel
		float sum(0.0f);
		for (std::int32_t t(0); t <= radius; ++t)
		{
			double const weight(0.3989422804 * std::exp(-0.5 * t * t / (sigma * sigma)) / sigma);
			m_kernel[radius + t] = static_cast<float>(weight);
			m_kernel[radius - t] = static_cast<float>(weight);
			if (t != 0)
				sum += static_cast<float>(weight) * 2.0f;
			else
				sum += static_cast<float>(weight);
		}
		// normalize kernels
		for (std::int32_t k(0); k != radius * 2 + 1; ++k)
			m_kernel[k] /= sum;

		D3D11_MAPPED_SUBRESOURCE mappedResource;

		// set kernel
		THROW(context->Map(m_kernelBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, std::addressof(mappedResource)));
		auto kernelBuf = static_cast<float*>(mappedResource.pData);
		std::uninitialized_copy_n(m_kernel, radius * 2 + 1, kernelBuf);
		context->Unmap(m_kernelBuffer, 0);

		// set gaussianBlurInfo
		THROW(context->Map(m_gaussianBlurInfoBuf, 0, D3D11_MAP_WRITE_DISCARD, 0, std::addressof(mappedResource)));
		auto gaussianBlurInfoBuf = static_cast<gaussianBlurInfo *>(mappedResource.pData);
		gaussianBlurInfoBuf->height = input.height;
		gaussianBlurInfoBuf->width = input.width;
		gaussianBlurInfoBuf->radius = radius;
		context->Unmap(m_gaussianBlurInfoBuf, 0);

		RGBAImageGPU horzOutput(device, input); // empty_like

		std::size_t groupX(((input.width - 1) / GroupThreads) + 1);
		std::size_t groupY(((input.height - 1) / GroupThreads) + 1);

		ID3D11ShaderResourceView *srvs[2] = { input.srv, m_kernelBufferSRV };
		context->CSSetShaderResources(0, 2, srvs);
		context->CSSetUnorderedAccessViews(0, 1, std::addressof(ans.uav), nullptr);
		context->CSSetUnorderedAccessViews(1, 1, std::addressof(horzOutput.uav), nullptr);
		context->CSSetConstantBuffers(0, 1, std::addressof(m_gaussianBlurInfoBuf));

		context->CSSetShader(m_csH, nullptr, 0);
		context->Dispatch(groupX, input.height, 1);

		context->CSSetShader(m_csV, nullptr, 0);
		context->Dispatch(input.width, groupY, 1);

		context->CSSetShader(nullptr, nullptr, 0);
		context->CSSetUnorderedAccessViews(0, 1, std::addressof(g_nullUAV), nullptr);
		context->CSSetUnorderedAccessViews(1, 1, std::addressof(g_nullUAV), nullptr);
		context->CSSetShaderResources(0, 1, std::addressof(g_nullSRV));
		context->CSSetShaderResources(1, 1, std::addressof(g_nullSRV));
		context->CSSetConstantBuffers(0, 1, std::addressof(g_nullCB));
	}
	RGBAImageGPU operator()(ID3D11Device *device, ID3D11DeviceContext *context, RGBAImageGPU const &input, std::uint32_t radius, double sigma)
	{
		RGBAImageGPU ans(device, input); // empty_like
		this->operator()(device, context, input, radius, sigma, ans);
		return ans;
	}

};
