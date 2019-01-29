#include "DX12Renderer.h"
#include "MaterialDX12.h"

using namespace Microsoft::WRL;

////////////////////////////////////////////////////
int DX12Renderer::initialize(unsigned int _width, unsigned int _height)
{
	CreateSDLWindow(_width, _height);
	CreateDevice();
	CreateCommandInterface();
	CreateSwapChain();
	CreateFenceAndEvent();
	CreateRenderTargets();
	SetViewportAndScissorRect(_width, _height);


	return 0;
}

void DX12Renderer::CreateSDLWindow(unsigned int _width, unsigned int _height)
{
	if(SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		fprintf(stderr, "%s", SDL_GetError());
		exit(-1);
	}

	m_pWindow = SDL_CreateWindow(
		"DX12", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, _width, _height, 0);
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

void DX12Renderer::CreateDevice()
{
	ComPtr<ID3D12Debug> debugController;

	if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
	}

	IDXGIFactory6* factory = nullptr;
	IDXGIAdapter1* adapter = nullptr;

	CreateDXGIFactory(IID_PPV_ARGS(&factory));
	//Loop through and find adapter
	for(UINT adapterIndex = 0;; ++adapterIndex)
	{
		adapter = nullptr;
		if(DXGI_ERROR_NOT_FOUND == factory->EnumAdapters1(adapterIndex, &adapter))
		{
			break;
		}

		if(SUCCEEDED(
			   D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, __uuidof(ID3D12Device), nullptr)))
		{
			break;
		}

		SafeRelease(&adapter);
	}

	if(adapter)
	{
		HRESULT hr = S_OK;
		ThrowIfFailed(
			hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_pDevice4)));

		SafeRelease(&adapter);
	}

	SafeRelease(&factory);
}

void DX12Renderer::CreateCommandInterface()
{
	D3D12_COMMAND_QUEUE_DESC cqd = {};
	m_pDevice4->CreateCommandQueue(&cqd, IID_PPV_ARGS(&m_pCommandQueue));

	m_pDevice4->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
									   IID_PPV_ARGS(&m_pCommandAllocator));

	m_pDevice4->CreateCommandList(0,
								  D3D12_COMMAND_LIST_TYPE_DIRECT,
								  m_pCommandAllocator,
								  nullptr,
								  IID_PPV_ARGS(&m_pCommandList3));
	m_pCommandList3->Close();
}

void DX12Renderer::CreateSwapChain(int _width, int _height)
{
	IDXGIFactory5* factory = nullptr;
	CreateDXGIFactory(IID_PPV_ARGS(&factory));

	//Swap chain incoming
	DXGI_SWAP_CHAIN_DESC1 scDesc = {};
	scDesc.Width = _width;
	scDesc.Height = _height;
	scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scDesc.Stereo = false;
	scDesc.SampleDesc.Count = 1;
	scDesc.SampleDesc.Quality = 0;
	scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scDesc.BufferCount = m_NUM_BACK_BUFFERS;
	scDesc.Scaling = DXGI_SCALING_NONE;
	scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	scDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	scDesc.Flags = 0;

	IDXGISwapChain1* swapChain1 = nullptr;
	ThrowIfFailed(factory->CreateSwapChainForHwnd(
		m_pCommandQueue, GetActiveWindow(), &scDesc, nullptr, nullptr, &swapChain1));

	ThrowIfFailed(swapChain1->QueryInterface(IID_PPV_ARGS(&m_pSwapChain4)));

	SafeRelease(&factory);
}

void DX12Renderer::CreateFenceAndEvent()
{
	m_pDevice4->CreateFence(m_fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence));
	m_fenceValue = 1;

	m_EventHandle = CreateEvent(0, false, false, 0);
}

void DX12Renderer::CreateRenderTargets()
{
	D3D12_DESCRIPTOR_HEAP_DESC dhd = {};
	dhd.NumDescriptors = m_NUM_BACK_BUFFERS;
	dhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	ThrowIfFailed(m_pDevice4->CreateDescriptorHeap(&dhd, IID_PPV_ARGS(&m_pRenderTargetsHeap)));

	m_RenderTargetDescriptorSize =
		m_pDevice4->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_pRenderTargetsHeap->GetCPUDescriptorHandleForHeapStart();

	for(UINT i = 0; i < m_NUM_BACK_BUFFERS; i++)
	{
		ThrowIfFailed(m_pSwapChain4->GetBuffer(i, IID_PPV_ARGS(&m_pRenderTargets[i])));
		m_pDevice4->CreateRenderTargetView(m_pRenderTargets[i], nullptr, cdh);
		cdh.ptr += m_RenderTargetDescriptorSize;
	}
}

void DX12Renderer::SetViewportAndScissorRect(int _width, int _height)
{
	m_Viewport.TopLeftX = 0.0f;
	m_Viewport.TopLeftY = 0.0f;
	m_Viewport.Width = (float)_width;
	m_Viewport.Height = (float)_height;
	m_Viewport.MinDepth = 0.0f;
	m_Viewport.MaxDepth = 1.0f;

	m_ScissorRect.left = (long)0;
	m_ScissorRect.right = (long)_width;
	m_ScissorRect.top = (long)0;
	m_ScissorRect.bottom = (long)_height;
}

////////////////////////////////////////////////////
Material* DX12Renderer::makeMaterial(const std::string& _name)
{
	return new MaterialDX12();
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
	return "";
}

////////////////////////////////////////////////////
std::string DX12Renderer::getShaderExtension()
{
	return "";
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
