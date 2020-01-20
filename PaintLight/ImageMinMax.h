#pragma once

#include <tuple>
#include <numeric>
#include "DXUT.h"
#include <wrl.h>

#include "RGBAImage.h"

template<std::size_t max_image_size = 8192>
class ImageMinMax
{
private:
	std::size_t const GroupThreads = 1024;
	std::size_t const totalGroupX = ((max_image_size - 1) / GroupThreads) + 1;
	std::size_t const totalGroupY = ((max_image_size - 1) / GroupThreads) + 1;
	ID3D11ComputeShader *m_csRow;
	ID3D11ComputeShader *m_csCol;
	ID3D11Buffer *m_buf;

	ID3D11Buffer *resultMaxRowBuf;
	ID3D11Buffer *resultMinRowBuf;
	ID3D11UnorderedAccessView *resultMaxRowBufUAV;
	ID3D11UnorderedAccessView *resultMinRowBufUAV;

	ID3D11Buffer *resultMaxBuf;
	ID3D11Buffer *resultMinBuf;
	ID3D11UnorderedAccessView *resultMaxBufUAV;
	ID3D11UnorderedAccessView *resultMinBufUAV;

	struct reductionInfo {
		std::uint32_t width, height;
		std::uint32_t x_groups, y_groups;
	};
public:
	ImageMinMax() :
		m_csRow(nullptr),
		m_csCol(nullptr),
		m_buf(nullptr),
		resultMaxRowBuf(nullptr),
		resultMinRowBuf(nullptr),
		resultMaxBuf(nullptr),
		resultMinBuf(nullptr),
		resultMaxRowBufUAV(nullptr),
		resultMinRowBufUAV(nullptr),
		resultMaxBufUAV(nullptr),
		resultMinBufUAV(nullptr)
	{
	}
	ImageMinMax(ID3D11Device *device, ID3D11DeviceContext *context) :
		m_csRow(nullptr),
		m_csCol(nullptr),
		m_buf(nullptr),
		resultMaxRowBuf(nullptr),
		resultMinRowBuf(nullptr),
		resultMaxBuf(nullptr),
		resultMinBuf(nullptr),
		resultMaxRowBufUAV(nullptr),
		resultMinRowBufUAV(nullptr),
		resultMaxBufUAV(nullptr),
		resultMinBufUAV(nullptr)
	{
		ID3DBlob *csByteCodesRow{ nullptr };
		THROW(CompileShader(L"ImageMinMax.hlsl", nullptr, "reduceRow", "cs_5_0", std::addressof(csByteCodesRow)));
		THROW(device->CreateComputeShader(csByteCodesRow->GetBufferPointer(), csByteCodesRow->GetBufferSize(), nullptr, std::addressof(m_csRow)));

		ID3DBlob *csByteCodesCol{ nullptr };
		THROW(CompileShader(L"ImageMinMax.hlsl", nullptr, "reduceCol", "cs_5_0", std::addressof(csByteCodesCol)));
		THROW(device->CreateComputeShader(csByteCodesCol->GetBufferPointer(), csByteCodesCol->GetBufferSize(), nullptr, std::addressof(m_csCol)));

		D3D11_BUFFER_DESC bufDesc;
		bufDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufDesc.MiscFlags = 0;
		bufDesc.ByteWidth = sizeof(reductionInfo);

		THROW(device->CreateBuffer(std::addressof(bufDesc), nullptr, std::addressof(m_buf)));


		// create buffer to store row result
		D3D11_BUFFER_DESC desc = {};
		desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
		desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		desc.ByteWidth = sizeof(float) * 4 * (totalGroupX * max_image_size);
		desc.StructureByteStride = sizeof(float) * 4;

		THROW(device->CreateBuffer(std::addressof(desc), nullptr, std::addressof(resultMaxRowBuf)));
		THROW(device->CreateBuffer(std::addressof(desc), nullptr, std::addressof(resultMinRowBuf)));

		D3D11_UNORDERED_ACCESS_VIEW_DESC descUAV = {};
		descUAV.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		descUAV.Buffer.FirstElement = 0;
		descUAV.Format = DXGI_FORMAT_UNKNOWN;
		descUAV.Buffer.NumElements = desc.ByteWidth / desc.StructureByteStride;

		THROW(device->CreateUnorderedAccessView(resultMaxRowBuf, std::addressof(descUAV), std::addressof(resultMaxRowBufUAV)));
		THROW(device->CreateUnorderedAccessView(resultMinRowBuf, std::addressof(descUAV), std::addressof(resultMinRowBufUAV)));



		// create buffer to store results
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		desc.ByteWidth = sizeof(float) * 4 * (totalGroupX * totalGroupY);

		THROW(device->CreateBuffer(std::addressof(desc), nullptr, std::addressof(resultMaxBuf)));
		THROW(device->CreateBuffer(std::addressof(desc), nullptr, std::addressof(resultMinBuf)));

		descUAV.Buffer.NumElements = desc.ByteWidth / desc.StructureByteStride;

		THROW(device->CreateUnorderedAccessView(resultMaxBuf, std::addressof(descUAV), std::addressof(resultMaxBufUAV)));
		THROW(device->CreateUnorderedAccessView(resultMinBuf, std::addressof(descUAV), std::addressof(resultMinBufUAV)));
	}

	void Release() noexcept
	{
		try {
			SAFE_RELEASE(m_csRow);
			SAFE_RELEASE(m_csCol);
			SAFE_RELEASE(m_buf);
			SAFE_RELEASE(resultMaxRowBuf);
			SAFE_RELEASE(resultMinRowBuf);
			SAFE_RELEASE(resultMaxBuf);
			SAFE_RELEASE(resultMinBuf);
			SAFE_RELEASE(resultMaxRowBufUAV);
			SAFE_RELEASE(resultMinRowBufUAV);
			SAFE_RELEASE(resultMaxBufUAV);
			SAFE_RELEASE(resultMinBufUAV);
		}
		catch (...) {

		}
	}

	~ImageMinMax()
	{
		Release();
	}

	ImageMinMax(ImageMinMax const &other) = delete;
	ImageMinMax &operator=(ImageMinMax const &other) = delete;

	ImageMinMax(ImageMinMax &&other) noexcept :
		m_csRow(other.m_csRow),
		m_csCol(other.m_csCol),
		m_buf(other.m_buf),
		resultMaxRowBuf(other.resultMaxRowBuf),
		resultMinRowBuf(other.resultMinRowBuf),
		resultMaxBuf(other.resultMaxBuf),
		resultMinBuf(other.resultMinBuf),
		resultMaxRowBufUAV(other.resultMaxRowBufUAV),
		resultMinRowBufUAV(other.resultMinRowBufUAV),
		resultMaxBufUAV(other.resultMaxBufUAV),
		resultMinBufUAV(other.resultMinBufUAV)
	{
		other.m_csRow = nullptr;
		other.m_csCol = nullptr;
		other.m_buf = nullptr;
		resultMaxRowBuf = nullptr;
		resultMinRowBuf = nullptr;
		resultMaxBuf = nullptr;
		resultMinBuf = nullptr;
		resultMaxRowBufUAV = nullptr;
		resultMinRowBufUAV = nullptr;
		resultMaxBufUAV = nullptr;
		resultMinBufUAV = nullptr;
	}
	ImageMinMax &operator=(ImageMinMax &&other) noexcept
	{
		if (std::addressof(other) != this)
		{
			Release();
			m_csRow = other.m_csRow;
			m_csCol = other.m_csCol;
			m_buf = other.m_buf;
			resultMaxRowBuf = other.resultMaxRowBuf;
			resultMinRowBuf = other.resultMinRowBuf;
			resultMaxBuf = other.resultMaxBuf;
			resultMinBuf = other.resultMinBuf;
			resultMaxRowBufUAV = other.resultMaxRowBufUAV;
			resultMinRowBufUAV = other.resultMinRowBufUAV;
			resultMaxBufUAV = other.resultMaxBufUAV;
			resultMinBufUAV = other.resultMinBufUAV;
			other.m_csRow = nullptr;
			other.m_csCol = nullptr;
			other.m_buf = nullptr;
			other.resultMaxRowBuf = nullptr;
			other.resultMinRowBuf = nullptr;
			other.resultMaxBuf = nullptr;
			other.resultMinBuf = nullptr;
			other.resultMaxRowBufUAV = nullptr;
			other.resultMinRowBufUAV = nullptr;
			other.resultMaxBufUAV = nullptr;
			other.resultMinBufUAV = nullptr;
		}
		return *this;
	}
public:
	std::tuple<std::tuple<float, float, float>, std::tuple<float, float, float>> RunCPU(ID3D11Device *device, ID3D11DeviceContext *context, const RGBAImageGPU &img)
	{
		RGBAImage imgCPU(img.Download(device, context));
		float maxR(std::numeric_limits<float>::min());
		float minR(std::numeric_limits<float>::max());
		float maxG(std::numeric_limits<float>::min());
		float minG(std::numeric_limits<float>::max());
		float maxB(std::numeric_limits<float>::min());
		float minB(std::numeric_limits<float>::max());

		float *src(imgCPU.data);
		for (std::size_t i(0); i != imgCPU.width * imgCPU.height; ++i, src += 4)
		{
			float r(src[0]), g(src[1]), b(src[2]);
			maxR = std::max(maxR, r);
			maxG = std::max(maxG, g);
			maxB = std::max(maxB, b);
			minR = std::min(minR, r);
			minG = std::min(minG, g);
			minB = std::min(minB, b);
		}

		return { {minR,minG,minB},{maxR,maxG,maxB} };
	}
	std::tuple<std::tuple<float, float, float>, std::tuple<float, float, float>> operator()(ID3D11Device *device, ID3D11DeviceContext *context, const RGBAImageGPU &input)
	{
		std::size_t const groupX(((input.width - 1) / GroupThreads) + 1);
		std::size_t const groupY(((input.height - 1) / GroupThreads) + 1);
		D3D11_MAPPED_SUBRESOURCE mappedResource;

		// set reductionInfo
		THROW(context->Map(m_buf, 0, D3D11_MAP_WRITE_DISCARD, 0, std::addressof(mappedResource)));
		auto reductionInfoBuf = static_cast<reductionInfo *>(mappedResource.pData);
		reductionInfoBuf->height = input.height;
		reductionInfoBuf->width = input.width;
		reductionInfoBuf->x_groups = totalGroupX;
		reductionInfoBuf->y_groups = totalGroupY;
		context->Unmap(m_buf, 0);

		// execute
		ID3D11UnorderedAccessView *uavs[] = { resultMaxRowBufUAV, resultMinRowBufUAV, resultMaxBufUAV, resultMinBufUAV };
		context->CSSetUnorderedAccessViews(0, 4, uavs, nullptr); // set UAVs for results

		context->CSSetShaderResources(0, 1, std::addressof(input.srv)); // set input SRV
		context->CSSetConstantBuffers(0, 1, std::addressof(m_buf)); // set CB

		context->CSSetShader(m_csRow, nullptr, 0);
		context->Dispatch(groupX, input.height, 1);

		context->CSSetShader(m_csCol, nullptr, 0);
		context->Dispatch(groupX, groupY, 1);

		context->CSSetShader(nullptr, nullptr, 0);
		ID3D11UnorderedAccessView *uavs_null[] = { nullptr, nullptr, nullptr, nullptr };
		context->CSSetUnorderedAccessViews(0, 4, uavs_null, nullptr);
		context->CSSetShaderResources(0, 1, std::addressof(g_nullSRV));
		context->CSSetConstantBuffers(0, 1, std::addressof(g_nullCB));


		// reduce in CPU
		float maxR(std::numeric_limits<float>::min());
		float minR(std::numeric_limits<float>::max());
		float maxG(std::numeric_limits<float>::min());
		float minG(std::numeric_limits<float>::max());
		float maxB(std::numeric_limits<float>::min());
		float minB(std::numeric_limits<float>::max());

		THROW(context->Map(resultMaxBuf, 0, D3D11_MAP_READ, 0, std::addressof(mappedResource)));
		auto resultMaxBase(static_cast<float const*>(mappedResource.pData));
		for (std::size_t i(0); i != groupY; ++i)
		{
			auto resultMax(resultMaxBase + (4 * i * totalGroupX));
			for (std::size_t j(0); j != groupX; ++j, resultMax += 4) {
				float r(resultMax[0]), g(resultMax[1]), b(resultMax[2]);
				maxR = std::max(maxR, r);
				maxG = std::max(maxG, g);
				maxB = std::max(maxB, b);
			}
		}
		context->Unmap(resultMaxBuf, 0);

		THROW(context->Map(resultMinBuf, 0, D3D11_MAP_READ, 0, std::addressof(mappedResource)));
		auto resultMinBase(static_cast<float const *>(mappedResource.pData));
		for (std::size_t i(0); i != groupY; ++i)
		{
			auto resultMin(resultMinBase + (4 * i * totalGroupX));
			for (std::size_t j(0); j != groupX; ++j, resultMin += 4) {
				float r(resultMin[0]), g(resultMin[1]), b(resultMin[2]);
				minR = std::min(minR, r);
				minG = std::min(minG, g);
				minB = std::min(minB, b);
			}
		}
		context->Unmap(resultMinBuf, 0);

		return { {minR,minG,minB},{maxR,maxG,maxB} };
	}
};
