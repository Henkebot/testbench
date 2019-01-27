#include "DX12Renderer.h"
using namespace Microsoft::WRL;
////////////////////////////////////////////////////
int DX12Renderer::initialize(unsigned int _width, unsigned int _height)
{
	// Window init
	if(SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		fprintf(stderr, "%s", SDL_GetError());
		return 1;
	}

	m_pWindow = SDL_CreateWindow(
		"DirectX 12", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, _width, _height, 0);

	// Enable Debug Layer
	{
		ComPtr<ID3D12Debug> debugController;

		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));

		debugController->EnableDebugLayer();
	}

	// Create factory controller
	ComPtr<IDXGIFactory4> dxgiFactory;
	ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&dxgiFactory)));

	// Create device
	{
		ComPtr<IDXGIAdapter1> adapter(_getHardwareAdapter(dxgiFactory.Get()));

		ThrowIfFailed(
			D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_cDevice)));
	}

	// Command Queue
	{
		D3D12_COMMAND_QUEUE_DESC commandDesc;
		commandDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		commandDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		commandDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		commandDesc.NodeMask = 0;

		ThrowIfFailed(m_cDevice->CreateCommandQueue(&commandDesc, IID_PPV_ARGS(&m_cCommandQueue)));
	}

	// SwapChain
	{
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.Width = _width;
		swapChainDesc.Height = _height;
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.Stereo = FALSE;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = FRAME_COUNT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		swapChainDesc.Flags = 0;

		HWND windowHandle = GetActiveWindow();

		// CreateSwapChainForHwnd only accepts a swapChain1
		// and we need a swapchain3 to get backbuffer index
		// so we create one with swapchain1 and transfer the
		// result to swapchain3
		ComPtr<IDXGISwapChain1> tempSwapChain;
		ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(m_cCommandQueue.Get(),
														  windowHandle,
														  &swapChainDesc,
														  nullptr,
														  nullptr,
														  &tempSwapChain));

		// Remove ALT+Enter resizing
		ThrowIfFailed(dxgiFactory->MakeWindowAssociation(windowHandle, DXGI_MWA_NO_ALT_ENTER));

		tempSwapChain.As(&m_cSwapChain);
		m_FrameIndex = m_cSwapChain->GetCurrentBackBufferIndex();
	}

	// Create render targets
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FRAME_COUNT;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	}
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
IDXGIAdapter1* DX12Renderer::_getHardwareAdapter(IDXGIFactory2* _pFactory) const
{
	ComPtr<IDXGIAdapter1> pAdapter = nullptr;

	for(UINT adapterIndex = 0;
		_pFactory->EnumAdapters1(adapterIndex, &pAdapter) != DXGI_ERROR_NOT_FOUND;
		adapterIndex++)
	{
		DXGI_ADAPTER_DESC1 adaptDesc;
		pAdapter->GetDesc1(&adaptDesc);
		printf("%ls", adaptDesc.Description);
		if(adaptDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			continue;

		if(SUCCEEDED(D3D12CreateDevice(
			   pAdapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)))
		{
			break;
		}
	}

	return pAdapter.Detach();
}

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
