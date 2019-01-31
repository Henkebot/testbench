#include "MaterialDX12.h"
#include "ConstantBufferDX12.h"
#include <fstream>
#include <sstream>
#include <vector>

////////////////////////////////////////////////////
MaterialDX12::MaterialDX12(ID3D12Device4* _device)
	: Material()
	, m_pDevice(_device)
{}

////////////////////////////////////////////////////
void MaterialDX12::setShader(const std::string& _shaderFileName, ShaderType _type)
{
	//if(shaderFileNames.find(_type) != shaderFileNames.end())
	//{
	//	removeShader(_type);
	//}
	shaderFileNames[_type] = _shaderFileName;
}

////////////////////////////////////////////////////
void MaterialDX12::removeShader(ShaderType _type) {}

////////////////////////////////////////////////////
void MaterialDX12::setDiffuse(Color _c) {}

////////////////////////////////////////////////////
int MaterialDX12::compileMaterial(std::string& _errString)
{

	_compileShader(ShaderType::VS);
	_compileShader(ShaderType::PS);

	return 0;
}

void MaterialDX12::_compileShader(ShaderType _type)
{
	std::vector<D3D_SHADER_MACRO> macros;
	std::vector<std::string> Names;
	std::vector<std::string> Values;

	for(auto shaderDefine : shaderDefines[_type])
	{
		std::stringstream ss(shaderDefine);

		while(ss.good())
		{
			std::string define, Name, Value;
			ss >> define >> Name >> Value;

			// If name is empty no need to continue
			if(!strcmp(Name.c_str(), ""))
				break;

			Names.push_back(Name);
			Values.push_back(Value);
		}
	}
	for(int i = 0; i < Names.size(); i++)
	{
		D3D_SHADER_MACRO macro;
		macro.Name		 = Names[i].c_str();
		macro.Definition = Values[i].c_str();
		macros.push_back(macro);
	}

	D3D_SHADER_MACRO end;
	end.Name	   = NULL;
	end.Definition = NULL;
	macros.push_back(end);

	std::string shaderCode;
	{
		std::ifstream shaderFile(shaderFileNames[_type]);
		if(shaderFile.is_open())
		{
			shaderCode = std::string((std::istreambuf_iterator<char>(shaderFile)),
									 std::istreambuf_iterator<char>());
		}
	}

	LPCVOID pSrcData				 = (LPCVOID)shaderCode.data();
	SIZE_T SrcDataSize				 = shaderCode.size();
	LPCSTR pSourceName				 = shaderFileNames[_type].c_str();
	CONST D3D_SHADER_MACRO* pDefines = (CONST D3D_SHADER_MACRO*)&macros[0];
	ID3DInclude* pInclude			 = NULL;
	LPCSTR pEntrypoint				 = _entryPoint(_type);
	LPCSTR pTarget					 = _targetName(_type);
	UINT Flags1						 = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
	UINT Flags2						 = 0;
	ID3DBlob** ppCode				 = &m_pVertexBlob;
	ID3DBlob* ppErrorMsgs			 = nullptr;

	ThrowIfFailed(D3DCompile(pSrcData,
							 SrcDataSize,
							 pSourceName,
							 pDefines,
							 pInclude,
							 pEntrypoint,
							 pTarget,
							 Flags1,
							 Flags2,
							 ppCode,
							 &ppErrorMsgs));
	if(ppErrorMsgs)
	{
		OutputDebugStringA((char*)ppErrorMsgs->GetBufferPointer());
		ppErrorMsgs->Release();
	}
}

LPCSTR MaterialDX12::_targetName(ShaderType _type)
{
	switch(_type)
	{
	case ShaderType::VS:
		return "vs_5_0";
	case ShaderType::PS:
		return "ps_5_0";
	default:
		return nullptr;
	}
}

LPCSTR MaterialDX12::_entryPoint(ShaderType _type)
{
	switch(_type)
	{
	case ShaderType::VS:
		return "VS_Main";
	case ShaderType::PS:
		return "PS_Main";
	default:
		return nullptr;
	}
}

////////////////////////////////////////////////////
void MaterialDX12::addConstantBuffer(std::string name, unsigned int location)
{
	constantBuffers[location] = new ConstantBufferDX12(m_pDevice, name, location);
}

////////////////////////////////////////////////////
void MaterialDX12::updateConstantBuffer(const void* data, size_t size, unsigned int location)
{
	constantBuffers[location]->setData(data, size, this, location);
}

////////////////////////////////////////////////////
int MaterialDX12::enable()
{
	return 0;
}

////////////////////////////////////////////////////
void MaterialDX12::disable() {}
