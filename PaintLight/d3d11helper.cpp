
#include "DXUT.h"
#include "d3d11helper.h"

ID3D11UnorderedAccessView *const g_nullUAV = nullptr;
ID3D11ShaderResourceView *const g_nullSRV = nullptr;
ID3D11Buffer *const g_nullCB = nullptr;

HRESULT CompileShader(LPCWSTR pFileName,
	const D3D_SHADER_MACRO *pDefines,
	LPCSTR pEntrypoint, LPCSTR pTarget,
	ID3DBlob **ppCode)
{
	DWORD dwShaderFlags{ D3DCOMPILE_ENABLE_STRICTNESS };
#if defined( DEBUG ) || defined( _DEBUG )
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG;
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	dwShaderFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

	ID3DBlob *pErrorBlob = nullptr;

	HRESULT const hr{ D3DCompileFromFile(pFileName, pDefines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
					   pEntrypoint, pTarget, dwShaderFlags, 0,
					   ppCode, &pErrorBlob) };
	if (pErrorBlob)
	{
		OutputDebugStringA(static_cast<char const *>(pErrorBlob->GetBufferPointer()));
		SAFE_RELEASE(pErrorBlob);
	}
	return hr;
}
