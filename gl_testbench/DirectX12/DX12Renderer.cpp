#include "DX12Renderer.h"

////////////////////////////////////////////////////
int DX12Renderer::initialize(unsigned int _width, unsigned int _height)
{
	if(SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		fprintf(stderr, "%s", SDL_GetError());
		return 1;
	}

	m_pWindow = SDL_CreateWindow(
		"DirectX 12", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, _width, _height, 0);

	return 0;
}

////////////////////////////////////////////////////
void DX12Renderer::setWinTitle(const char* _title) {}

////////////////////////////////////////////////////
void DX12Renderer::present() {}

////////////////////////////////////////////////////
int DX12Renderer::shutdown()
{
	return 0;
}

////////////////////////////////////////////////////
void DX12Renderer::setClearColor(float _r, float _g, float _b, float _a) {}

////////////////////////////////////////////////////
void DX12Renderer::clearBuffer(unsigned int _mask) {}

////////////////////////////////////////////////////
void DX12Renderer::setRenderState(RenderState* _renderState) {}

////////////////////////////////////////////////////
void DX12Renderer::submit(Mesh* _mesh) {}

////////////////////////////////////////////////////
void DX12Renderer::frame() {}

////////////////////////////////////////////////////
Material* DX12Renderer::makeMaterial(const std::string& _name)
{
	return nullptr;
}

////////////////////////////////////////////////////
Mesh* DX12Renderer::makeMesh()
{
	return nullptr;
}

////////////////////////////////////////////////////
VertexBuffer* DX12Renderer::makeVertexBuffer(size_t _size, VertexBuffer::DATA_USAGE _usage)
{
	return nullptr;
}

////////////////////////////////////////////////////
Texture2D* DX12Renderer::makeTexture2D()
{
	return nullptr;
}

////////////////////////////////////////////////////
Sampler2D* DX12Renderer::makeSampler2D()
{
	return nullptr;
}

////////////////////////////////////////////////////
RenderState* DX12Renderer::makeRenderState()
{
	return nullptr;
}

////////////////////////////////////////////////////
std::string DX12Renderer::getShaderPath()
{
	return std::string();
}

////////////////////////////////////////////////////
std::string DX12Renderer::getShaderExtension()
{
	return std::string();
}

////////////////////////////////////////////////////
ConstantBuffer* DX12Renderer::makeConstantBuffer(std::string _name, unsigned int _location)
{
	return nullptr;
}

////////////////////////////////////////////////////
Technique* DX12Renderer::makeTechnique(Material* _material, RenderState* _renderState)
{
	return nullptr;
}
