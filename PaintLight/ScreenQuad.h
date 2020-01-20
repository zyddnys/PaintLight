#pragma once

#include <array>

#include "DXUT.h"
#include "d3d11helper.h"

class ScreenQuad
{
private:
	struct PosTex
	{
		float x, y;
		float u, v;
	};

	static std::array<PosTex, 4> constexpr ScreenQuadVertices{
		PosTex{-1, 1, 0, 0},
		PosTex{1, 1, 1, 0},
		PosTex{-1, -1, 0, 1},
		PosTex{1, -1, 1, 1},
	};

	static std::array<UINT, 6> constexpr ScreenQuadIndices{2, 0, 1, 1, 3, 2};

	static std::array<D3D11_INPUT_ELEMENT_DESC, 2> constexpr ScreenQuadInputLayout
	{
		D3D11_INPUT_ELEMENT_DESC{ "POSITION",  0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		D3D11_INPUT_ELEMENT_DESC{ "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	struct screenQuadInfo
	{
		float rangeMin, rangeMax;
		float gamma;
		std::uint32_t pad1;
	};

	ID3D11Buffer *m_vertexBuffer;
	ID3D11Buffer *m_indexBuffer;

	ID3D11VertexShader *m_vs;
	ID3D11PixelShader *m_ps;

	ID3D11InputLayout *m_ia;

	ID3D11SamplerState *m_sampler;

	ID3D11Buffer *m_buf;

public:
	void Release() noexcept
	{
		try {
			SAFE_RELEASE(m_vertexBuffer);
			SAFE_RELEASE(m_indexBuffer);
			SAFE_RELEASE(m_vs);
			SAFE_RELEASE(m_ps);
			SAFE_RELEASE(m_ia);
			SAFE_RELEASE(m_sampler);
			SAFE_RELEASE(m_buf);
		}
		catch (...) {}
	}
public:
	ScreenQuad() noexcept:
		m_vertexBuffer(nullptr),
		m_indexBuffer(nullptr),
		m_vs(nullptr),
		m_ps(nullptr),
		m_ia(nullptr),
		m_sampler(nullptr),
		m_buf(nullptr)
	{}
	ScreenQuad(ID3D11Device *device)
	{
		D3D11_BUFFER_DESC vertexBufferDesc{}, indexBufferDesc{};
		D3D11_SUBRESOURCE_DATA vertexData{}, indexData{};

		vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		vertexBufferDesc.ByteWidth = sizeof(decltype(ScreenQuadVertices)::value_type) * ScreenQuadVertices.size();
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexBufferDesc.CPUAccessFlags = 0;
		vertexBufferDesc.MiscFlags = 0;
		vertexBufferDesc.StructureByteStride = 0;

		vertexData.pSysMem = ScreenQuadVertices.data();
		vertexData.SysMemPitch = 0;
		vertexData.SysMemSlicePitch = 0;

		THROW(device->CreateBuffer(&vertexBufferDesc, std::addressof(vertexData), std::addressof(m_vertexBuffer)));

		indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		indexBufferDesc.ByteWidth = sizeof(decltype(ScreenQuadIndices)::value_type) * ScreenQuadIndices.size();
		indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		indexBufferDesc.CPUAccessFlags = 0;
		indexBufferDesc.MiscFlags = 0;
		indexBufferDesc.StructureByteStride = 0;

		indexData.pSysMem = ScreenQuadIndices.data();
		indexData.SysMemPitch = 0;
		indexData.SysMemSlicePitch = 0;

		THROW(device->CreateBuffer(&indexBufferDesc, std::addressof(indexData), std::addressof(m_indexBuffer)));

		ID3DBlob *pVertexShaderBuffer{nullptr};
		ID3DBlob *pPixelShaderBuffer{nullptr};

		// compile VS
		THROW(CompileShader(L"ScreenQuadVS.hlsl", nullptr, "main", "vs_5_0", std::addressof(pVertexShaderBuffer)));

		// compile PS
		THROW(CompileShader(L"ScreenQuadPS.hlsl", nullptr, "main", "ps_5_0", std::addressof(pPixelShaderBuffer)));

		THROW(device->CreateVertexShader(pVertexShaderBuffer->GetBufferPointer(), pVertexShaderBuffer->GetBufferSize(), nullptr, std::addressof(m_vs)));
		THROW(device->CreatePixelShader(pPixelShaderBuffer->GetBufferPointer(), pPixelShaderBuffer->GetBufferSize(), nullptr, std::addressof(m_ps)));

		// create input layout
		THROW(device->CreateInputLayout(ScreenQuadInputLayout.data(), ScreenQuadInputLayout.size(),
			  pVertexShaderBuffer->GetBufferPointer(), pVertexShaderBuffer->GetBufferSize(), std::addressof(m_ia)));

		// create sampler for our texture
		D3D11_SAMPLER_DESC samplerDesc;
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
		samplerDesc.MipLODBias = 0.0f;
		samplerDesc.MaxAnisotropy = 1;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		samplerDesc.BorderColor[0] = 0;
		samplerDesc.BorderColor[1] = 0;
		samplerDesc.BorderColor[2] = 0;
		samplerDesc.BorderColor[3] = 0;
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

		// Create the texture sampler state.
		THROW(device->CreateSamplerState(&samplerDesc, &m_sampler));

		D3D11_BUFFER_DESC bufDesc;
		bufDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufDesc.MiscFlags = 0;
		bufDesc.ByteWidth = sizeof(screenQuadInfo);

		THROW(device->CreateBuffer(std::addressof(bufDesc), nullptr, std::addressof(m_buf)));
	}
	~ScreenQuad()
	{
		Release();
	}
	ScreenQuad(ScreenQuad const &other) = delete;
	ScreenQuad &operator=(ScreenQuad const &other) = delete;

	ScreenQuad(ScreenQuad &&other) noexcept :
		m_vertexBuffer(other.m_vertexBuffer),
		m_indexBuffer(other.m_indexBuffer),
		m_ps(other.m_ps),
		m_vs(other.m_vs),
		m_ia(other.m_ia),
		m_sampler(other.m_sampler),
		m_buf(other.m_buf)
	{
		other.m_vertexBuffer = nullptr;
		other.m_indexBuffer = nullptr;
		other.m_vs = nullptr;
		other.m_ps = nullptr;
		other.m_ia = nullptr;
		other.m_sampler = nullptr;
		other.m_buf = nullptr;
	}
	ScreenQuad &operator=(ScreenQuad &&other) noexcept
	{
		if (std::addressof(other) != this)
		{
			Release();
			m_vertexBuffer = other.m_vertexBuffer;
			m_indexBuffer = other.m_indexBuffer;
			m_vs = other.m_vs;
			m_ps = other.m_ps;
			m_ia = other.m_ia;
			m_sampler = other.m_sampler;
			m_buf = other.m_buf;
			other.m_vertexBuffer = nullptr;
			other.m_indexBuffer = nullptr;
			other.m_vs = nullptr;
			other.m_ps = nullptr;
			other.m_ia = nullptr;
			other.m_sampler = nullptr;
			other.m_buf = nullptr;
		}
		return *this;
	}
public:
	void Draw(ID3D11DeviceContext *context, ID3D11ShaderResourceView *srvTexture, float rangeMin = 0.0f, float rangeMax = 255.0f, float gamma = 1.0f)
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource;

		// set screenQuadInfo
		THROW(context->Map(m_buf, 0, D3D11_MAP_WRITE_DISCARD, 0, std::addressof(mappedResource)));
		auto screenQuadInfoBuf = static_cast<screenQuadInfo *>(mappedResource.pData);
		screenQuadInfoBuf->rangeMin = rangeMin;
		screenQuadInfoBuf->rangeMax = rangeMax;
		screenQuadInfoBuf->gamma = gamma;
		context->Unmap(m_buf, 0);

		unsigned int stride;
		unsigned int offset;

		// Set vertex buffer stride and offset.
		stride = sizeof(PosTex);
		offset = 0;

		// Setup IA
		context->IASetInputLayout(m_ia);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		context->IASetVertexBuffers(0, 1, std::addressof(m_vertexBuffer), std::addressof(stride), std::addressof(offset));
		context->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

		// Setup shaders
		context->VSSetShader(m_vs, nullptr, 0);
		context->PSSetShader(m_ps, nullptr, 0);
		
		// Setup sampler
		context->PSSetSamplers(0, 1, std::addressof(m_sampler));

		// Setup texture
		context->PSSetShaderResources(0, 1, std::addressof(srvTexture));

		// Setup CB
		context->PSSetConstantBuffers(0, 1, std::addressof(m_buf));

		// Draw
		context->DrawIndexed(6, 0, 0);
	}
};
