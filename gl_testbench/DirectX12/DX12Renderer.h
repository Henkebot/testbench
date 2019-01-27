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

private:
	SDL_Window* m_pWindow;

	FLOAT m_fClearColor[4];

	Microsoft::WRL::ComPtr<ID3D12Device> m_cDevice;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_cCommandQueue;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_cCommandList;
	Microsoft::WRL::ComPtr<IDXGISwapChain3> m_cSwapChain;

	static const UINT FRAME_COUNT = 2;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_cRTVHeap;
	UINT m_RTVDescriptorSize;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_cRenderTarget[FRAME_COUNT];
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_cCommandAllocator[FRAME_COUNT];

	UINT m_FrameIndex;
	UINT64 m_FenceValues[FRAME_COUNT];
	HANDLE m_FenceEvent;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_cFence;

private:
	IDXGIAdapter1* _getHardwareAdapter(IDXGIFactory2* _pFactory) const;
};