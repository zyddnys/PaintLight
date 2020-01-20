#pragma once

#include "DXUT.h"
#include "d3d11helper.h"
#include "RGBAImage.h"
#include "MulScalar.h"
#include "AddScalar.h"
#include "ImageMinMax.h"

class NormalizeImage
{
private:
	MulScalar m_mulScalar;
	AddScalar m_addScalar;
	ImageMinMax<> m_imageMinMax;
public:
	NormalizeImage()
	{
		;
	}
	NormalizeImage(ID3D11Device *device, ID3D11DeviceContext *context)
	{
		m_mulScalar = MulScalar(device, context);
		m_addScalar = AddScalar(device, context);
		m_imageMinMax = ImageMinMax(device, context);
	}

	void Release() noexcept
	{
		try {
			m_addScalar.Release();
			m_mulScalar.Release();
			m_imageMinMax.Release();
		}
		catch (...) {

		}
	}

	~NormalizeImage()
	{
		Release();
	}

	NormalizeImage(NormalizeImage const &other) = delete;
	NormalizeImage &operator=(NormalizeImage const &other) = delete;

	NormalizeImage(NormalizeImage &&other) noexcept :
		m_mulScalar(std::move(other.m_mulScalar)),
		m_addScalar(std::move(other.m_addScalar)),
		m_imageMinMax(std::move(other.m_imageMinMax))
	{
	}
	NormalizeImage &operator=(NormalizeImage &&other) noexcept
	{
		if (std::addressof(other) != this)
		{
			Release();
			m_mulScalar = std::move(other.m_mulScalar);
			m_addScalar = std::move(other.m_addScalar);
			m_imageMinMax = std::move(other.m_imageMinMax);
		}
		return *this;
	}
public:
	void operator()(ID3D11Device *device, ID3D11DeviceContext *context, RGBAImageGPU const &input, float maxValue, RGBAImageGPU &ans)
	{
		if (input != ans)
			throw std::runtime_error("input and output shape mismatch");
		auto const [minValues, maxValues] = m_imageMinMax(device, context, input);
		auto const [maxR, maxG, maxB] = maxValues;
		auto const [minR, minG, minB] = minValues;
		float scaleR(maxValue / (maxR - minR)), scaleG(maxValue / (maxG - minG)), scaleB(maxValue / (maxB - minB));
		auto subtracted(m_addScalar(device, context, input, -minR, -minG, -minB));
		auto normalized(m_mulScalar(device, context, subtracted, scaleR, scaleG, scaleB));

		ans = std::move(normalized);
	}
	RGBAImageGPU operator()(ID3D11Device *device, ID3D11DeviceContext *context, RGBAImageGPU const &input, float maxValue)
	{
		RGBAImageGPU ans(device, input); // empty_like
		this->operator()(device, context, input, maxValue, ans);
		return ans;
	}
};
