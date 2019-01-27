#include "DX12Renderer.h"
#include "MaterialDX12.h"

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
		ComPtr<IDXGIAdapter1> adapter = _getHardwareAdapter(dxgiFactory.Get());

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
		ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(
			m_cCommandQueue.Get(), windowHandle, &swapChainDesc, nullptr, nullptr, &tempSwapChain));

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

		ThrowIfFailed(m_cDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_cRTVHeap)));

		m_RTVDescriptorSize =
			m_cDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_cRTVHeap->GetCPUDescriptorHandleForHeapStart());
		// Create a RTV for each frame
		for(UINT i = 0; i < FRAME_COUNT; i++)
		{
			ThrowIfFailed(m_cSwapChain->GetBuffer(i, IID_PPV_ARGS(&m_cRenderTarget[i])));
			m_cDevice->CreateRenderTargetView(m_cRenderTarget[i].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(m_RTVDescriptorSize);
		}
	}

	// Create command allocators for each frame
	{
		for(int i = 0; i < FRAME_COUNT; i++)
		{
			ThrowIfFailed(m_cDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
															IID_PPV_ARGS(&m_cCommandAllocator[i])));
		}
	}

	// Create sunchronization objects
	{
		ThrowIfFailed(m_cDevice->CreateFence(
			m_FenceValues[m_FrameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_cFence)));

		// Increment the value, we are expected the next frame
		m_FenceValues[m_FrameIndex]++;

		m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if(nullptr == m_FenceEvent)
		{
			// Failed to createEvent
			return 1;
		}
	}

	// Create a dummy commandlist
	ThrowIfFailed(m_cDevice->CreateCommandList(0,
											   D3D12_COMMAND_LIST_TYPE_DIRECT,
											   m_cCommandAllocator[m_FrameIndex].Get(),
											   nullptr,
											   IID_PPV_ARGS(&m_cCommandList)));
	ThrowIfFailed(m_cCommandList->Close());
	return 0;
}

////////////////////////////////////////////////////
void DX12Renderer::setWinTitle(const char* _title)
{
	SDL_SetWindowTitle(m_pWindow, _title);
}

////////////////////////////////////////////////////
void DX12Renderer::present()
{
	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = {m_cCommandList.Get()};
	m_cCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	ThrowIfFailed(m_cSwapChain->Present(1, 0));

	// Move to next frame
	{
		// Schedule a signal command in the queue.
		const UINT64 currentFenceValue = m_FenceValues[m_FrameIndex];
		ThrowIfFailed(m_cCommandQueue->Signal(m_cFence.Get(), currentFenceValue));

		// Update the frame index
		m_FrameIndex = m_cSwapChain->GetCurrentBackBufferIndex();

		// If the next frame is not ready to be rendered yet, wait until it is ready.
		if(m_cFence->GetCompletedValue() < m_FenceValues[m_FrameIndex])
		{
			ThrowIfFailed(
				m_cFence->SetEventOnCompletion(m_FenceValues[m_FrameIndex], m_FenceEvent));

			WaitForSingleObjectEx(m_FenceEvent, INFINITE, FALSE);
		}

		// Set the fence value for the next frame.
		m_FenceValues[m_FrameIndex] = currentFenceValue + 1;
	}
}

////////////////////////////////////////////////////
int DX12Renderer::shutdown()
{
	return 0;
}

////////////////////////////////////////////////////
void DX12Renderer::setClearColor(float _r, float _g, float _b, float _a)
{
	m_fClearColor[0] = _r;
	m_fClearColor[1] = _g;
	m_fClearColor[2] = _b;
	m_fClearColor[3] = _a;
}

////////////////////////////////////////////////////
void DX12Renderer::clearBuffer(unsigned int _mask) {}

////////////////////////////////////////////////////
void DX12Renderer::setRenderState(RenderState* _renderState) {}

////////////////////////////////////////////////////
void DX12Renderer::submit(Mesh* _mesh) {}

////////////////////////////////////////////////////
void DX12Renderer::frame()
{
	/*	Command list allocators can only be reset when the associated
	command lists have finished execution on the GPU;  apps should
	use fences to determine GPU execution progress.
*/
	ThrowIfFailed(m_cCommandAllocator[m_FrameIndex]->Reset());

	/*
	However when ExecuteCommandList() is called on a particular command
	list, that command list can then be reset at any time and must be
	before re-recording
	*/

	ThrowIfFailed(m_cCommandList->Reset(m_cCommandAllocator[m_FrameIndex].Get(), nullptr));

	// Indicate that the backbuffer will be used as a render target.
	m_cCommandList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(m_cRenderTarget[m_FrameIndex].Get(),
											  D3D12_RESOURCE_STATE_PRESENT,
											  D3D12_RESOURCE_STATE_RENDER_TARGET));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
		m_cRTVHeap->GetCPUDescriptorHandleForHeapStart(), m_FrameIndex, m_RTVDescriptorSize);

	m_cCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	m_cCommandList->ClearRenderTargetView(rtvHandle, m_fClearColor, 0, nullptr);

	// Render stuff here

	// Indicate that the back buffer will now be used to present
	m_cCommandList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(m_cRenderTarget[m_FrameIndex].Get(),
											  D3D12_RESOURCE_STATE_RENDER_TARGET,
											  D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(m_cCommandList->Close());
}

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
	return "../assets/DX12/";
}

////////////////////////////////////////////////////
std::string DX12Renderer::getShaderExtension()
{
	return ".hlsl";
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
