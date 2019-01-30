#include "MaterialDX12.h"
#include <fstream>
#include <iostream> // Remove
#include <sstream>

////////////////////////////////////////////////////
MaterialDX12::MaterialDX12()
	: Material()
{}

////////////////////////////////////////////////////
void MaterialDX12::setShader(const std::string& _shaderFileName, ShaderType _type)
{
	// If the shader was found
	if(shaderFileNames.find(_type) != shaderFileNames.end())
	{
		removeShader(_type);
	}
	shaderFileNames[_type] = _shaderFileName;
}

////////////////////////////////////////////////////
void MaterialDX12::removeShader(ShaderType _type) {}

////////////////////////////////////////////////////
void MaterialDX12::setDiffuse(Color _c) {}

////////////////////////////////////////////////////
int MaterialDX12::compileMaterial(std::string& _errString)
{
	std::ifstream shaderFile(shaderFileNames[ShaderType::VS]);
	std::string shaderText;
	if(shaderFile.is_open())
	{
		shaderText = std::string((std::istreambuf_iterator<char>(shaderFile)),
								 std::istreambuf_iterator<char>());
		shaderFile.close();
	}
	else
	{
		_errString = "Cannot find file: " + shaderFileNames[ShaderType::VS];
		return -1;
	}

	auto defines = _getShaderDefines(ShaderType::VS);

	LPCVOID pSrcData				 = reinterpret_cast<LPCVOID>(shaderText.c_str());
	SIZE_T SrcDataSize				 = shaderText.size();
	LPCSTR pSourceName				 = nullptr;
	const D3D_SHADER_MACRO* pDefines = (const D3D_SHADER_MACRO*)&defines[0];
	ID3DInclude* pInclude			 = nullptr;
	LPCSTR pEntrypoint				 = _getEntryPoint(_type);
	LPCSTR pTarget					 = "vs_5_1";
	UINT Flags1						 = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
	UINT Flags2						 = 0;
	ID3DBlob** ppCode				 = &m_cCompiledCode[(int)ShaderType::VS];
	ID3DBlob* ppErrorMsgs			 = nullptr;

	if(FAILED(D3DCompile(pSrcData,
						 SrcDataSize,
						 pSourceName,
						 pDefines,
						 pInclude,
						 pEntrypoint,
						 pTarget,
						 Flags1,
						 Flags2,
						 ppCode,
						 &ppErrorMsgs)))
	{
		if(ppErrorMsgs)
		{
			OutputDebugStringA((char*)ppErrorMsgs->GetBufferPointer());
			ppErrorMsgs->Release();
		}

		if(*ppCode)
			(*ppCode)->Release();
	}
	return 0;
}

////////////////////////////////////////////////////
void MaterialDX12::addConstantBuffer(std::string name, unsigned int location) {}

////////////////////////////////////////////////////
void MaterialDX12::updateConstantBuffer(const void* data, size_t size, unsigned int location) {}

////////////////////////////////////////////////////
int MaterialDX12::enable()
{
	return 0;
}

////////////////////////////////////////////////////
void MaterialDX12::disable() {}

LPCSTR MaterialDX12::_getEntryPoint(ShaderType _type)
{
	switch(_type)
	{
	case ShaderType::VS:
		return "VS_Main";
	case ShaderType::PS:
		return "PS_Main";
	}
	return "Invalid";
}

LPCSTR MaterialDX12::_getTarget(ShaderType _type)
{
	switch(_type)
	{
	case ShaderType::VS:
		return "vs_5_0";
	case ShaderType::PS:
		return "ps_5_0";
	}
	return "Invalid";
}

int MaterialDX12::compileShader(ShaderType _type)
{
	std::string shaderText;
	{
		std::ifstream shaderFile(shaderFileNames[_type]);
		// Failed to open shader file
		if(false == shaderFile.is_open())
			return 1;

		shaderText = std::string((std::istreambuf_iterator<char>(shaderFile)),
								 std::istreambuf_iterator<char>());
	}

	auto defines = _getShaderDefines(_type);

	LPCVOID pSrcData				 = reinterpret_cast<LPCVOID>(shaderText.c_str());
	SIZE_T SrcDataSize				 = shaderText.size();
	LPCSTR pSourceName				 = nullptr;
	const D3D_SHADER_MACRO* pDefines = (const D3D_SHADER_MACRO*)&defines[0];
	ID3DInclude* pInclude			 = nullptr;
	LPCSTR pEntrypoint				 = _getEntryPoint(_type);
	LPCSTR pTarget					 = _getTarget(_type);
	UINT Flags1						 = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
	UINT Flags2						 = 0;
	ID3DBlob** ppCode				 = &m_cCompiledCode[(int)_type];
	ID3DBlob* ppErrorMsgs			 = nullptr;

	if(FAILED(D3DCompile(pSrcData,
						 SrcDataSize,
						 pSourceName,
						 pDefines,
						 pInclude,
						 pEntrypoint,
						 pTarget,
						 Flags1,
						 Flags2,
						 ppCode,
						 &ppErrorMsgs)))
	{
		if(ppErrorMsgs)
		{
			OutputDebugStringA((char*)ppErrorMsgs->GetBufferPointer());
			ppErrorMsgs->Release();
		}

		if(*ppCode)
			(*ppCode)->Release();
	}
	return 0;
}

std::vector<D3D_SHADER_MACRO> MaterialDX12::_getShaderDefines(ShaderType _type)
{
	//TODO(Henrik): I need to free the char's that i create in this function
	std::vector<D3D_SHADER_MACRO> macros;
	for(auto& def : shaderDefines[_type])
	{
		std::cout << def;
		std::stringstream stream(def);
		while(stream)
		{
			std::string define, name, definition;

			stream >> define >> name >> definition;

			if(strcmp(define.c_str(), "") == 0)
				break;

			D3D_SHADER_MACRO newMacro;

			newMacro.Name = new char[name.size() + 1];
			strcpy_s((char*)newMacro.Name, name.size() + 1, name.c_str());

			newMacro.Definition = new char[definition.size() + 1];
			strcpy_s((char*)newMacro.Definition, definition.size() + 1, definition.c_str());

			macros.push_back(newMacro);
		}
	}
	macros.push_back({});

	return macros;
}
