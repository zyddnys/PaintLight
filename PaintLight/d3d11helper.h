#pragma once

#include <stdexcept>
#include <exception>
#include <string>
#include <system_error>

#include "DXUT.h"

/*
class win32_error : public std::runtime_error
{
	inline static std::string format_get_last_error(std::uint32_t error)
	{
		if (error)
		{
			std::array<char, 32768> buffer;
			auto const buffer_length(win32::FormatMessageA(
				0x00000200 | 0x00001000,
				nullptr,
				error,
				(1 << 10),
				buffer.data(),
				buffer.size(),
				nullptr));
			if (buffer_length)
				return std::string(buffer.data(), buffer.data() + buffer_length);
		}
		return {};
	}
	std::uint32_t ec;
public:
	explicit win32_error(std::uint32_t error = win32::GetLastError()) :std::runtime_error(format_get_last_error(error)), ec(error) {}
	auto get() const noexcept
	{
		return ec;
	}
};
*/

#define THROW(x) { HRESULT const hr{x}; if( FAILED(hr) ) { throw std::runtime_error(std::system_category().message(hr)); } }

HRESULT CompileShader(LPCWSTR pFileName,
	const D3D_SHADER_MACRO *pDefines,
	LPCSTR pEntrypoint, LPCSTR pTarget,
	ID3DBlob **ppCode);

extern ID3D11UnorderedAccessView *const g_nullUAV;
extern ID3D11ShaderResourceView *const g_nullSRV;
extern ID3D11Buffer *const g_nullCB;
