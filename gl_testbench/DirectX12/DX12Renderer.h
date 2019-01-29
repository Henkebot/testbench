#pragma once

#include "../Renderer.h"
#include "DX12Common.h"
#include <SDL.h>

#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "SDL2main.lib")

class DX12Renderer : public Renderer
{
public:
	DX12Renderer() = default;
	virtual ~DX12Renderer() = default;

	Material* makeMaterial(const std::string& _name) override;
	Mesh* makeMesh() override;
	VertexBuffer* makeVertexBuffer(size_t _size, VertexBuffer::DATA_USAGE _usage) override;
	Texture2D* makeTexture2D() override;
	Sampler2D* makeSampler2D() override;
	RenderState* makeRenderState() override;
	std::string getShaderPath() override;
	std::string getShaderExtension() override;
	ConstantBuffer* makeConstantBuffer(std::string _name, unsigned int _location) override;
	Technique* makeTechnique(Material* _material, RenderState* _renderState) override;

	int initialize(unsigned int _width = 800, unsigned int _height = 600) override;
	void CreateSDLWindow(unsigned int _width, unsigned int _height);
	void setWinTitle(const char* _title) override;
	void present() override;
	int shutdown() override;

	void setClearColor(float _r, float _g, float _b, float _a) override;
	void clearBuffer(unsigned int _mask);
	// can be partially overriden by a specific Technique.
	void setRenderState(RenderState* _renderState) override;
	// submit work (to render) to the renderer.
	void submit(Mesh* _mesh) override;
	void frame() override;

	void CreateDevice();

private:
	void CreateCommandInterface();
	void CreateSwapChain(int _width = 0, int _height = 0);
	void CreateFenceAndEvent();
	void CreateRenderTargets();
	void SetViewportAndScissorRect(int _width, int _height);

private:
	static const unsigned int m_NUM_BACK_BUFFERS = 2U;
	SDL_Window* m_pWindow = nullptr;
	ID3D12Device4* m_pDevice4 = nullptr;
	ID3D12GraphicsCommandList3* m_pCommandList3 = nullptr;

	ID3D12CommandQueue* m_pCommandQueue = nullptr;
	ID3D12CommandAllocator* m_pCommandAllocator = nullptr;
	IDXGISwapChain4* m_pSwapChain4 = nullptr;

	ID3D12Fence1* m_pFence = nullptr;
	int m_fenceValue = 0;
	HANDLE m_EventHandle = nullptr;

	ID3D12DescriptorHeap* m_pRenderTargetsHeap = nullptr;
	UINT m_RenderTargetDescriptorSize = 0;

	ID3D12Resource* m_pRenderTargets[m_NUM_BACK_BUFFERS];

	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;
};