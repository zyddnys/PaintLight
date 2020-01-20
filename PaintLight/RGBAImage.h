#pragma once

#include <stdexcept>

#include "DXUT.h"
#include "d3d11helper.h"

//#define cimg_use_cpp11
//#define cimg_use_opencv
//#include "CImg.h"
#include <atlbase.h>
#include <wincodec.h>

class RGBAImage // FP32 from 0.0f to 255.0f, RGBARGBARGBA...
{
	//TODO: maybe add alignment?
public:
	float *data;
	std::uint32_t width, height;
public:
	std::tuple<std::uint32_t, std::uint32_t> GetSize() { return { width,height }; }
	float *GetRawData() { return data; }
public:
	void Setup(std::uint32_t width, std::uint32_t height)
	{
		float *new_data(new float[height * width * 4]{ 255.0f });
		Release();
		this->width = width;
		this->height = height;
		this->data = new_data;
	}
	std::tuple<float, float, float> At(std::uint32_t i, std::uint32_t j)
	{
		float *pos(data + (i * width + j) * 4);
		return { pos[0], pos[1], pos[2] };
	}
	void Set(std::uint32_t i, std::uint32_t j, float r2, float g2, float b2)
	{
		float *pos(data + (i * width + j) * 4);
		pos[0] = r2;
		pos[1] = g2;
		pos[2] = b2;
	}
public:
	RGBAImage() :data(nullptr), width(0), height(0)
	{

	}
	RGBAImage(LPCWSTR filename) :data(nullptr), width(0), height(0)
	{
		//CoInitialize(nullptr);
		{
			CComPtr<IWICImagingFactory> pFactory;
			CComPtr<IWICBitmapDecoder> pDecoder;
			CComPtr<IWICBitmapFrameDecode> pFrame;
			CComPtr<IWICFormatConverter> pFormatConverter;

			CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, (LPVOID *)&pFactory);

			pFactory->CreateDecoderFromFilename(filename, NULL, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &pDecoder);
			// Check the return value to see if:
			//  - The specified file exists and can be read.
			//  - A decoder is found for this file format.

			UINT frameCount = 0;
			pDecoder->GetFrameCount(&frameCount);

			pDecoder->GetFrame(0, &pFrame);
			// The zero-based index should be smaller than the frame count.

			pFrame->GetSize(&width, &height);

			WICPixelFormatGUID pixelFormatGUID;
			pFrame->GetPixelFormat(&pixelFormatGUID);

			// The frame can use many different pixel formats.
			// You can copy the raw pixel values by calling "pFrame->CopyPixels( )".
			// This method needs a buffer that can hold all bytes.
			// The total number of bytes is: width x height x bytes per pixel

			// The disadvantage of this solution is that you have to deal with all possible pixel formats.

			// You can make your life easy by converting the frame to a pixel format of
			// your choice. The code below shows how to convert the pixel format to 24-bit RGB.

			pFactory->CreateFormatConverter(&pFormatConverter);

			pFormatConverter->Initialize(pFrame,                       // Input bitmap to convert
				GUID_WICPixelFormat24bppRGB,  // Destination pixel format
				WICBitmapDitherTypeNone,      // Specified dither pattern
				nullptr,                      // Specify a particular palette
				0.f,                          // Alpha threshold
				WICBitmapPaletteTypeCustom); // Palette translation type

			UINT bytesPerPixel = 3; // Because we have converted the frame to 24-bit RGB
			UINT stride = width * bytesPerPixel;
			UINT size = width * height * bytesPerPixel; // The size of the required memory buffer for
														// holding all the bytes of the frame.

			std::vector<BYTE> bitmap(size); // The buffer to hold all the bytes of the frame.
			pFormatConverter->CopyPixels(NULL, stride, size, bitmap.data());

			// Note: the WIC COM pointers should be released before 'CoUninitialize( )' is called.
			data = new float[width * height * 4];

			float *dst(data);
			BYTE const *src = bitmap.data();
			for (std::size_t i(0); i < width * height; ++i, src += 3, dst += 4)
			{
				dst[0] = static_cast<float>(src[0]);
				dst[1] = static_cast<float>(src[1]);
				dst[2] = static_cast<float>(src[2]);
				dst[3] = 255.0f;
			}
		}
		//CoUninitialize();
	}
	void Release() noexcept
	{
		try {
			if (data) {
				delete[] data;
				data = nullptr;
			}
			width = height = 0;
		}
		catch (...) {}
	}
	~RGBAImage()
	{
		Release();
	}

	RGBAImage(RGBAImage const &other) :data(nullptr), width(other.width), height(other.height)
	{
		data = new float[width * height * 4];
		std::uninitialized_copy_n(other.data, width * height * 4, data);
	}

	RGBAImage &operator=(RGBAImage const &other)
	{
		if (std::addressof(other) != this)
		{
			float *new_data(new float[other.height * other.width * 4]);
			std::uninitialized_copy_n(other.data, other.width * other.height * 4, new_data);
			Release();
			data = new_data;
			width = other.height;
			height = other.height;
		}
		return *this;
	}

	RGBAImage(RGBAImage &&other) noexcept :data(other.data), width(other.width), height(other.height)
	{
		other.data = nullptr;
		other.width = 0;
		other.height = 0;
	}

	RGBAImage &operator=(RGBAImage &&other) noexcept
	{
		if (std::addressof(other) != this)
		{
			Release();
			data = other.data;
			width = other.width;
			height = other.height;

			other.data = nullptr;
			other.width = 0;
			other.height = 0;
		}
		return *this;
	}
	operator bool() const noexcept
	{
		return width != 0 && height != 0 && data != nullptr;
	}
};

class RGBAImageGPU // upload RGBAImage to GPU and create SRV/UAV/Texture2D object
{
public:
	std::uint32_t width, height;
public:
	ID3D11Texture2D *tex;
	ID3D11ShaderResourceView *srv;
	ID3D11UnorderedAccessView *uav;
public:
	RGBAImageGPU() noexcept: width(0), height(0), tex(nullptr), srv(nullptr), uav(nullptr)
	{

	}
	RGBAImageGPU(RGBAImage const &img, ID3D11Device *device)
	{
		width = img.width;
		height = img.height;

		// create Texture2D
		D3D11_TEXTURE2D_DESC textureDesc{};
		textureDesc.Width = width;
		textureDesc.Height = height;
		textureDesc.MipLevels = 1;
		textureDesc.ArraySize = 1;
		textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.Usage = D3D11_USAGE_DEFAULT;
		textureDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
		textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		textureDesc.MiscFlags = 0;
		THROW(device->CreateTexture2D(&textureDesc, nullptr, std::addressof(tex)));

		// create SRV
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = textureDesc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		THROW(device->CreateShaderResourceView(tex, std::addressof(srvDesc), std::addressof(srv)));

		// create UAV
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
		uavDesc.Format = textureDesc.Format;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;
		THROW(device->CreateUnorderedAccessView(tex, std::addressof(uavDesc), std::addressof(uav)));
	}
	RGBAImageGPU(RGBAImage const &img, ID3D11Device *device, ID3D11DeviceContext *context)
	{
		width = img.width;
		height = img.height;

		// create Texture2D
		D3D11_TEXTURE2D_DESC textureDesc{};
		textureDesc.Width = width;
		textureDesc.Height = height;
		textureDesc.MipLevels = 1;
		textureDesc.ArraySize = 1;
		textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.Usage = D3D11_USAGE_DEFAULT;
		textureDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
		textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		textureDesc.MiscFlags = 0;
		THROW(device->CreateTexture2D(&textureDesc, nullptr, std::addressof(tex)));

		// create SRV
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = textureDesc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		THROW(device->CreateShaderResourceView(tex, std::addressof(srvDesc), std::addressof(srv)));

		// create UAV
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
		uavDesc.Format = textureDesc.Format;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;
		THROW(device->CreateUnorderedAccessView(tex, std::addressof(uavDesc), std::addressof(uav)));

		// upload data
		Upload(img, device, context);
	}
	void Release() noexcept
	{
		try {
			SAFE_RELEASE(tex);
			SAFE_RELEASE(srv);
			SAFE_RELEASE(uav);
		}
		catch (...) {
		}
	}
	~RGBAImageGPU()
	{
		Release();
	}

	// empty_like
	RGBAImageGPU(ID3D11Device *device, RGBAImageGPU const &other)
	{
		width = other.width;
		height = other.height;

		// create Texture2D
		D3D11_TEXTURE2D_DESC textureDesc{};
		textureDesc.Width = width;
		textureDesc.Height = height;
		textureDesc.MipLevels = 1;
		textureDesc.ArraySize = 1;
		textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.Usage = D3D11_USAGE_DEFAULT;
		textureDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
		textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		textureDesc.MiscFlags = 0;
		THROW(device->CreateTexture2D(&textureDesc, nullptr, std::addressof(tex)));

		// create SRV
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		THROW(device->CreateShaderResourceView(tex, std::addressof(srvDesc), std::addressof(srv)));

		// create UAV
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
		uavDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;
		THROW(device->CreateUnorderedAccessView(tex, std::addressof(uavDesc), std::addressof(uav)));
	}
	RGBAImageGPU(RGBAImageGPU const &other) = delete;
	RGBAImageGPU &operator=(RGBAImageGPU const &other) = delete;

	RGBAImageGPU(RGBAImageGPU &&other) noexcept: width(other.width), height(other.height), tex(other.tex), srv(other.srv), uav(other.uav)
	{
		other.width = 0;
		other.height = 0;
		other.tex = nullptr;
		other.srv = nullptr;
		other.uav = nullptr;
	}
	RGBAImageGPU &operator=(RGBAImageGPU &&other) noexcept
	{
		if (std::addressof(other) != this)
		{
			Release();
			width = other.width;
			height = other.height;
			tex = other.tex;
			srv = other.srv;
			uav = other.uav;
			other.width = 0;
			other.height = 0;
			other.tex = nullptr;
			other.srv = nullptr;
			other.uav = nullptr;
		}
		return *this;
	}
	operator bool()
	{
		return width != 0 && height != 0;
	}
public:
	void RebuildSRV(ID3D11Device *device)
	{
		SAFE_RELEASE(srv);
		// create SRV
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		THROW(device->CreateShaderResourceView(tex, std::addressof(srvDesc), std::addressof(srv)));
	}
public:
	void Upload(RGBAImage const &img, ID3D11Device *device, ID3D11DeviceContext *context)
	{
		ID3D11Texture2D *texTmp;

		// create Texture2D for upload
		D3D11_TEXTURE2D_DESC textureDesc{};
		textureDesc.Width = width;
		textureDesc.Height = height;
		textureDesc.MipLevels = 1;
		textureDesc.ArraySize = 1;
		textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.Usage = D3D11_USAGE_DYNAMIC;
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		textureDesc.MiscFlags = 0;
		THROW(device->CreateTexture2D(&textureDesc, nullptr, std::addressof(texTmp)));

		D3D11_MAPPED_SUBRESOURCE mappedResource;

		THROW(context->Map(texTmp, 0, D3D11_MAP_WRITE_DISCARD, 0, std::addressof(mappedResource)));

		BYTE const *buffer(static_cast<BYTE const *>(static_cast<void const *>(img.data)));
		auto rowspan(img.width * 4 * sizeof(float));
		BYTE *mappedData = static_cast<BYTE *>(mappedResource.pData);
		for (std::size_t i(0); i < img.height; ++i)
		{
			memcpy(mappedData, buffer, rowspan);
			mappedData += mappedResource.RowPitch;
			buffer += rowspan;
		}

		context->Unmap(texTmp, 0);

		context->CopyResource(tex, texTmp);

		texTmp->Release();
	}

	RGBAImage Download(ID3D11Device *device, ID3D11DeviceContext *context) const
	{
		ID3D11Texture2D *texTmp;

		// create Texture2D for upload
		D3D11_TEXTURE2D_DESC textureDesc{};
		textureDesc.Width = width;
		textureDesc.Height = height;
		textureDesc.MipLevels = 1;
		textureDesc.ArraySize = 1;
		textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.Usage = D3D11_USAGE_STAGING;
		textureDesc.BindFlags = 0;
		textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		textureDesc.MiscFlags = 0;
		THROW(device->CreateTexture2D(&textureDesc, nullptr, std::addressof(texTmp)));

		context->CopyResource(texTmp, tex);

		RGBAImage ret;
		ret.Setup(width, height);
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		HRESULT hr = context->Map(texTmp, 0, D3D11_MAP_READ, 0, std::addressof(mappedResource));
		std::size_t rowspan(width * 4 * sizeof(float));
		BYTE *mappedData = static_cast<BYTE *>(mappedResource.pData);
		BYTE *buffer(static_cast<BYTE *>(static_cast<void *>(ret.data)));
		for (std::size_t i(0); i < height; ++i)
		{
			memcpy(buffer, mappedData, rowspan);
			mappedData += mappedResource.RowPitch;
			buffer += rowspan;
		}
		context->Unmap(texTmp, 0);

		texTmp->Release();

		return ret;
	}
};

bool operator==(RGBAImage const &a, RGBAImage const &b)
{
	return a.width == b.width && a.height == b.height;
}

bool operator!=(RGBAImage const &a, RGBAImage const &b)
{
	return !(a == b);
}

bool operator==(RGBAImageGPU const &a, RGBAImageGPU const &b)
{
	return a.width == b.width && a.height == b.height;
}

bool operator!=(RGBAImageGPU const &a, RGBAImageGPU const &b)
{
	return !(a == b);
}
