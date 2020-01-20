#pragma once

#include <tuple>
#include <stdexcept>

#include "DXUT.h"
#include "d3d11helper.h"
#include "QuickHull.hpp"
#include "RGBAImage.h"

//#include "CoarseLighting.h"
#include "Lighting.h"
#include "NormalizeImage.h"
#include "GaussianBlur.h"
#include "AddScalar.h"
#include "MulScalar.h"
#include "MulImage.h"

using vec3f = quickhull::Vector3<float>;
#define CULLING

template<typename T = float>
std::tuple<bool, vec3f> rayTriangleIntersect(
	const vec3f &orig, const vec3f &dir,
	const vec3f &v0, const vec3f &v1, const vec3f &v2
	)
{
	T t, u, v;
	auto v0v1 = v1 - v0;
	auto v0v2 = v2 - v0;
	auto pvec = dir.crossProduct(v0v2);
	float det = v0v1.dotProduct(pvec);
#ifdef CULLING 
	// if the determinant is negative the triangle is backfacing
	// if the determinant is close to 0, the ray misses the triangle
	if (det < 1e-8f) return { false,vec3f(0,0,0) };
#else 
	// ray and triangle are parallel if det is close to 0
	if (fabs(det) < 1e-8f) return { false,vec3f(0,0,0) };
#endif 
	T invDet = 1.0 / det;

	auto tvec = orig - v0;
	u = tvec.dotProduct(pvec) * invDet;
	if (u < 0 || u > 1) return { false,vec3f(0,0,0) };

	auto qvec = tvec.crossProduct(v0v1);
	v = dir.dotProduct(qvec) * invDet;
	if (v < 0 || u + v > 1) return { false,vec3f(0,0,0) };

	t = v0v2.dotProduct(qvec) * invDet;

	return { true,orig + dir * t };
}

class PaintLight
{
public:
	float gamma;
	float ambient;
	float light_x, light_y, light_z;
	std::uint32_t blur_width;
	float blur_sigma;
	float pixel_scale, light_scale;
	float gamma_correction;
public:
	RGBAImage original;
	RGBAImage palette;
	RGBAImage stroke_density; // same value for all three channels
	RGBAImage blurred_image;
	RGBAImage normalized_image;
	RGBAImage coarse_lighting;
	RGBAImage refined_lighting;
	RGBAImage final_lighting;
	RGBAImage result;

	RGBAImageGPU original_GPU;
	RGBAImageGPU palette_GPU;
	RGBAImageGPU stroke_density_GPU; // same value for all three channels
	RGBAImageGPU blurred_image_GPU;
	RGBAImageGPU normalized_image_GPU;
	RGBAImageGPU coarse_lighting_GPU;
	RGBAImageGPU refined_lighting_GPU;
	RGBAImageGPU final_lighting_GPU;
	RGBAImageGPU result_GPU;
private:
	Lighting m_Lighting;
	NormalizeImage m_NormalizeImage;
	GaussianBlur<> m_GaussianBlur;
	AddScalar m_AddScalar;
	MulScalar m_MulScalar;
	MulImage m_MulImage;
public:
	void ReleaseImages() noexcept
	{
		original.Release();
		palette.Release();
		stroke_density.Release();
		blurred_image.Release();
		normalized_image.Release();
		coarse_lighting.Release();
		refined_lighting.Release();
		final_lighting.Release();
		result.Release();

		original_GPU.Release();
		palette_GPU.Release();
		stroke_density_GPU.Release();
		blurred_image_GPU.Release();
		normalized_image_GPU.Release();
		coarse_lighting_GPU.Release();
		refined_lighting_GPU.Release();
		final_lighting_GPU.Release();
		result_GPU.Release();
	}
	void Release() noexcept
	{
		ReleaseImages();

		m_Lighting.Release();
		m_NormalizeImage.Release();
		m_GaussianBlur.Release();
		m_AddScalar.Release();
		m_MulScalar.Release();
		m_MulImage.Release();
	}

	PaintLight() :gamma(1.0f), ambient(0.55), light_x(0.0f), light_y(0.0f), light_z(1.0f), blur_width(64), blur_sigma(16.0f), pixel_scale(1.0f), light_scale(10.0f), gamma_correction(1.0f)
	{

	}
	PaintLight(ID3D11Device *device, ID3D11DeviceContext *context) :
		gamma(1.0f),
		ambient(0.55),
		light_x(0.0f),
		light_y(0.0f),
		light_z(1.0f),
		blur_width(64),
		blur_sigma(16.0f),
		pixel_scale(1.0f),
		light_scale(10.0f),
		gamma_correction(1.0f)
	{
		m_Lighting = Lighting(device, context);
		m_NormalizeImage = NormalizeImage(device, context);
		m_GaussianBlur = GaussianBlur(device, context);
		m_AddScalar = AddScalar(device, context);
		m_MulScalar = MulScalar(device, context);
		m_MulImage = MulImage(device, context);
	}
	~PaintLight()
	{

	}
	PaintLight(PaintLight const &other) = delete;
	PaintLight &operator=(PaintLight const &other) = delete;

	PaintLight(PaintLight &&other) :
		gamma(other.gamma),
		ambient(other.ambient),
		light_x(other.light_x),
		light_y(other.light_y),
		light_z(other.light_z),
		blur_width(other.blur_width),
		blur_sigma(other.blur_sigma),
		pixel_scale(other.pixel_scale),
		light_scale(other.light_scale),
		gamma_correction(other.gamma_correction),

		original(std::move(other.original)),
		palette(std::move(other.palette)),
		stroke_density(std::move(other.stroke_density)),
		blurred_image(std::move(other.blurred_image)),
		normalized_image(std::move(other.normalized_image)),
		coarse_lighting(std::move(other.coarse_lighting)),
		refined_lighting(std::move(other.refined_lighting)),
		final_lighting(std::move(other.final_lighting)),
		result(std::move(other.result)),

		original_GPU(std::move(other.original_GPU)),
		palette_GPU(std::move(other.palette_GPU)),
		stroke_density_GPU(std::move(other.stroke_density_GPU)),
		blurred_image_GPU(std::move(other.blurred_image_GPU)),
		normalized_image_GPU(std::move(other.normalized_image_GPU)),
		coarse_lighting_GPU(std::move(other.coarse_lighting_GPU)),
		refined_lighting_GPU(std::move(other.refined_lighting_GPU)),
		final_lighting_GPU(std::move(other.final_lighting_GPU)),
		result_GPU(std::move(other.result_GPU)),

		m_Lighting(std::move(other.m_Lighting)),
		m_NormalizeImage(std::move(other.m_NormalizeImage)),
		m_GaussianBlur(std::move(other.m_GaussianBlur)),
		m_AddScalar(std::move(other.m_AddScalar)),
		m_MulScalar(std::move(other.m_MulScalar)),
		m_MulImage(std::move(other.m_MulImage))
	{

	}
	PaintLight &operator=(PaintLight &&other)
	{
		if (std::addressof(other) != this)
		{
			gamma = other.gamma;
			ambient = other.ambient;
			light_x = other.light_x;
			light_y = other.light_y;
			light_z = other.light_z;
			blur_width = other.blur_width;
			blur_sigma = other.blur_sigma;
			pixel_scale = other.pixel_scale;
			light_scale = other.light_scale;
			gamma_correction = other.gamma_correction;

			original = std::move(other.original);
			palette = std::move(other.palette);
			stroke_density = std::move(other.stroke_density);
			blurred_image = std::move(other.blurred_image);
			normalized_image = std::move(other.normalized_image);
			coarse_lighting = std::move(other.coarse_lighting);
			refined_lighting = std::move(other.refined_lighting);
			final_lighting = std::move(other.final_lighting);
			result = std::move(other.result);
			
			original_GPU = std::move(other.original_GPU);
			palette_GPU = std::move(other.palette_GPU);
			stroke_density_GPU = std::move(other.stroke_density_GPU);
			blurred_image_GPU = std::move(other.blurred_image_GPU);
			normalized_image_GPU = std::move(other.normalized_image_GPU);
			coarse_lighting_GPU = std::move(other.coarse_lighting_GPU);
			refined_lighting_GPU = std::move(other.refined_lighting_GPU);
			final_lighting_GPU = std::move(other.final_lighting_GPU);
			result_GPU = std::move(other.result_GPU);

			m_Lighting = std::move(other.m_Lighting);
			m_NormalizeImage = std::move(other.m_NormalizeImage);
			m_GaussianBlur = std::move(other.m_GaussianBlur);
			m_AddScalar = std::move(other.m_AddScalar);
			m_MulScalar = std::move(other.m_MulScalar);
			m_MulImage = std::move(other.m_MulImage);
		}
		return *this;
	}
public:
	operator bool() const
	{
		return original;
	}
public:
	void ComputeStrokeDensityCPU(ID3D11Device *device, ID3D11DeviceContext *context)
	{
		if (!original)
			throw std::runtime_error("empty image");
		auto const [width, height] = original.GetSize();
		quickhull::QuickHull<float> qh; // Could be double as well
		std::vector<vec3f> pointCloud;
		pointCloud.reserve(width * height);

		float *src = original.GetRawData();
		for (std::size_t i(0); i < width * height; ++i, src += 4)
			pointCloud.emplace_back(src[0], src[1], src[2]);

		auto hull = qh.getConvexHull(pointCloud, true, false);
		auto indexBuffer = hull.getIndexBuffer();
		auto vertexBuffer = hull.getVertexBuffer();

		float total_area(0.0f);
		vec3f centroid(0.0f, 0.0f, 0.0f);
		for (std::size_t i(0); i < indexBuffer.size(); i += 3)
		{
			auto const vertex1(vertexBuffer[indexBuffer[i + 0]]);
			auto const vertex2(vertexBuffer[indexBuffer[i + 1]]);
			auto const vertex3(vertexBuffer[indexBuffer[i + 2]]);
			auto const center((vertex1 + vertex2 + vertex3) / 3.0f);
			auto area((vertex2 - vertex1).crossProduct(vertex3 - vertex1).getLength() * 0.5f);
			centroid += center * area;
			total_area += area;
		}
		centroid /= total_area;


		std::size_t i(0);
		std::size_t const total(width * height);
		//wchar_t buf[256];
		//swprintf_s(buf, 255, L"faces: %d\n", indexBuffer.size() / 3);
		//OutputDebugString(buf);

		// calculate palette values
		palette.Setup(width, height);
		i = 0;
#pragma omp for schedule(dynamic, 1)
		for (; i < total; ++i)
		{
			std::size_t const x(i % width);
			std::size_t const y(i / width);

			auto const [r, g, b] = original.At(y, x);
			vec3f const val(r, g, b);
			vec3f dir(val - centroid);
			dir.normalize();
			bool hit_found(false);
			for (std::size_t f(0); f < indexBuffer.size(); f += 3)
			{
				auto const vertex1(vertexBuffer[indexBuffer[f + 0]]);
				auto const vertex2(vertexBuffer[indexBuffer[f + 1]]);
				auto const vertex3(vertexBuffer[indexBuffer[f + 2]]);

				auto const [hit, hit_point] = rayTriangleIntersect(centroid, dir, vertex1, vertex2, vertex3);
				if (hit)
				{
					palette.Set(y, x, hit_point.x, hit_point.y, hit_point.z);
					hit_found = true;
					break;
				}
			}
			if (!hit_found)
			{
				if (x > 0)
				{
					auto const [r, g, b] = palette.At(y, x - 1);
					palette.Set(y, x, r, g, b);
				}
				else if (y > 0)
				{
					auto const [r, g, b] = palette.At(y - 1, x);
					palette.Set(y, x, r, g, b);
				}
				else
				{
					OutputDebugString(L"warn\n");
				}
			}
		}

		// calculate stroke density
		stroke_density.Setup(width, height);
		i = 0;
#pragma omp for schedule(dynamic, 1)
		for (; i < total; ++i)
		{
			std::size_t const x(i % width);
			std::size_t const y(i / width);

			auto const [r1, g1, b1] = original.At(y, x);
			vec3f const p(r1, g1, b1);

			auto const [r2, g2, b2] = palette.At(y, x);
			vec3f const h(r2, g2, b2);

			auto const pixel_distance((p - centroid).getLength());
			auto const intersect_distance((h - centroid).getLength());
			float k(1.0f - std::fabs(1.0f - pixel_distance / intersect_distance));
			//k = std::sqrt(1.0f - k * k);
			stroke_density.Set(y, x, k, k, k);
		}

		// upload images to GPU
		palette_GPU.Upload(palette, device, context); // range 0 to 255
		stroke_density_GPU.Upload(stroke_density, device, context); // range 0 to 1

		//auto tmp(m_GaussianBlur(device, context, stroke_density_GPU, 3, 1.0f));
		//stroke_density_GPU = std::move(tmp);
	}

	void ComputeStrokeDensityGPU(ID3D11Device *device, ID3D11DeviceContext *context)
	{
		if (!original)
			throw std::runtime_error("empty image");
	}

	void ResetWithNewImage(ID3D11Device *device, ID3D11DeviceContext *context, std::wstring_view filename)
	{
		ReleaseImages();
		original = RGBAImage(filename.data());

		// phase 2 resources
		original_GPU = RGBAImageGPU(original, device);
		palette_GPU = RGBAImageGPU(original, device);
		stroke_density_GPU = RGBAImageGPU(original, device);

		// phase 1 resources
		blurred_image_GPU = RGBAImageGPU(original, device);
		normalized_image_GPU = RGBAImageGPU(original, device);
		coarse_lighting_GPU = RGBAImageGPU(original, device);
		refined_lighting_GPU = RGBAImageGPU(original, device);
		final_lighting_GPU = RGBAImageGPU(original, device);
		result_GPU = RGBAImageGPU(original, device);

		original_GPU.Upload(original, device, context);
	}
public:
	void operator()(ID3D11Device *device, ID3D11DeviceContext *context)
	{
		// step 1 blur image
		m_GaussianBlur(device, context, original_GPU, blur_width, blur_sigma, blurred_image_GPU); // range 0 to 255
		// step 2 calculate lighting effect
		m_Lighting(device, context, blurred_image_GPU, stroke_density_GPU, light_x, light_y, light_z, pixel_scale, gamma_correction, refined_lighting_GPU); // range 0 to 1
		// step 4 multiply by gamma
		auto tmp_gamma(m_MulScalar(device, context, refined_lighting_GPU, gamma, gamma, gamma)); // range 0 to gamma
		// step 5 add ambient
		m_AddScalar(device, context, tmp_gamma, ambient, ambient, ambient, final_lighting_GPU); // range ambient to gamma + ambient
		// step 6 multiply final lighting
		m_MulImage(device, context, original_GPU, final_lighting_GPU, result_GPU);
	}
};
