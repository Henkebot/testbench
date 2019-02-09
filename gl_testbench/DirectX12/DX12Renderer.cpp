#include "DX12Renderer.h"
#include "ConstantBufferDX12.h"
#include "MaterialDX12.h"
#include "MeshDX12.h"
#include "RenderStateDX12.h"
#include "TechniqueDX12.h"
#include "Texture2DDX12.h"
#include "VertexBufferDX12.h"
#include "Sampler2DDX12.h"

using namespace Microsoft::WRL;

static float depthValue = 0.0f;

////////////////////////////////////////////////////
int DX12Renderer::initialize(unsigned int _width, unsigned int _height)
{
	CreateSDLWindow(_width, _height);
	CreateDevice();
	CreateCommandInterface();
	CreateSwapChain();
	CreateFenceAndEvent();
	CreateDescriptorHeaps();
	CreateRenderTargets();
	CreateDepthStencil(_width, _height);
	SetViewportAndScissorRect(_width, _height);
	CreateRootSignature();

	// Remove these
	//CreateShadersAndPipeLineState();
	//CreateConstantBufferResources();
	//CreateTriangleData();

	WaitForGPU();

	return 0;
}

void DX12Renderer::CreateDepthStencil(unsigned int _width, unsigned int _height)
{
	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format						   = DXGI_FORMAT_D32_FLOAT;
	depthStencilDesc.ViewDimension				   = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags						   = D3D12_DSV_FLAG_NONE;

	D3D12_CLEAR_VALUE depthOptimizedClearValue	= {};
	depthOptimizedClearValue.Format				  = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth   = depthValue;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	ThrowIfFailed(m_pDevice4->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT,
									  _width,
									  _height,
									  1,
									  0,
									  1,
									  0,
									  D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(&m_pDepthResource)));
	;

	m_pDevice4->CreateDepthStencilView(m_pDepthResource.Get(),
									   &depthStencilDesc,
									   m_pDSVHeap->GetCPUDescriptorHandleForHeapStart());
}

void DX12Renderer::CreateDescriptorHeaps()
{
	// Render target heaps
	D3D12_DESCRIPTOR_HEAP_DESC dhd = {};
	dhd.NumDescriptors			   = NUM_BACK_BUFFERS;
	dhd.Type					   = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	ThrowIfFailed(m_pDevice4->CreateDescriptorHeap(&dhd, IID_PPV_ARGS(&m_pRenderTargetsHeap)));

	// Depth Stencil heaps
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors			   = 1;
	dsvHeapDesc.Type					   = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags					   = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(m_pDevice4->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_pDSVHeap)));

	// SRV heaps
	/*
	0 = pos
	1 = normal
	2 = uv
	3 = translate
	4 = tint
	5 = texture
	*/
	D3D12_DESCRIPTOR_HEAP_DESC heapDescriptorDesc = {};
	heapDescriptorDesc.NumDescriptors			  = 6;
	heapDescriptorDesc.Flags					  = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDescriptorDesc.Type						  = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	m_pDevice4->CreateDescriptorHeap(&heapDescriptorDesc, IID_PPV_ARGS(&m_pSRVHeap));
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
void DX12Renderer::setWinTitle(const char* _title)
{
	SDL_SetWindowTitle(m_pWindow, _title);
}

////////////////////////////////////////////////////
void DX12Renderer::present()
{
	DXGI_PRESENT_PARAMETERS pp = {};
	m_pSwapChain4->Present1(0, 0, &pp);
	WaitForGPU();
}

////////////////////////////////////////////////////
int DX12Renderer::shutdown()
{
	SDL_Quit();
	return 0;
}

////////////////////////////////////////////////////
void DX12Renderer::setClearColor(float _r, float _g, float _b, float _a)
{
	m_ClearColor[0] = _r;
	m_ClearColor[1] = _g;
	m_ClearColor[2] = _b;
	m_ClearColor[3] = _a;
}

////////////////////////////////////////////////////
void DX12Renderer::clearBuffer(unsigned int _mask) {}

////////////////////////////////////////////////////
void DX12Renderer::setRenderState(RenderState* _renderState) {}

////////////////////////////////////////////////////
void DX12Renderer::submit(Mesh* _mesh)
{
	drawList2[_mesh->technique].push_back(_mesh);
}

////////////////////////////////////////////////////
void DX12Renderer::frame()
{

	m_pCommandAllocator->Reset();
	m_pCommandList3->Reset(m_pCommandAllocator.Get(), nullptr);

	ID3D12DescriptorHeap* descriptorHeaps[] = {m_pSRVHeap.Get()};
	m_pCommandList3->SetDescriptorHeaps(ARRAYSIZE(descriptorHeaps), descriptorHeaps);

	m_pCommandList3->SetGraphicsRootSignature(m_pRootSignature.Get());

	m_pCommandList3->SetGraphicsRootDescriptorTable(
		0, m_pSRVHeap->GetGPUDescriptorHandleForHeapStart());

	m_pCommandList3->RSSetViewports(1, &m_Viewport);
	m_pCommandList3->RSSetScissorRects(1, &m_ScissorRect);

	UINT backBufferIndex			= m_pSwapChain4->GetCurrentBackBufferIndex();
	D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_pRenderTargetsHeap->GetCPUDescriptorHandleForHeapStart();
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_pDSVHeap->GetCPUDescriptorHandleForHeapStart());

	SetResourceTransitionBarrier(m_pCommandList3.Get(),
								 m_pRenderTargets[backBufferIndex].Get(),
								 D3D12_RESOURCE_STATE_PRESENT,
								 D3D12_RESOURCE_STATE_RENDER_TARGET);

	cdh.ptr += m_RenderTargetDescriptorSize * backBufferIndex;

	m_pCommandList3->OMSetRenderTargets(1, &cdh, true, &dsvHandle);

	m_pCommandList3->ClearRenderTargetView(cdh, m_ClearColor, 0, nullptr);
	m_pCommandList3->ClearDepthStencilView(
		dsvHandle, D3D12_CLEAR_FLAG_DEPTH, depthValue, 0, 0, nullptr);

	m_pCommandList3->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	int counter = 0;
	//printf("///////////////////////////////////////////\n");
	for(auto list : drawList2)
	{

		// list.first = techinique
		// list.second = vector<mesh>
		list.first->enable(this);

		for(auto mesh : list.second)
		{

			mesh->txBuffer->bind(nullptr);

			size_t numElem  = mesh->geometryBuffers[POSITION].numElements;
			size_t offset   = mesh->geometryBuffers[POSITION].offset;
			size_t sizeElem = mesh->geometryBuffers[POSITION].sizeElement;
			size_t index	= (offset / sizeElem) / 3;

			m_pCommandList3->DrawInstanced(numElem, 1, offset, 0);
			//printf("%zu\n", index);

			//for(auto t : mesh->textures)
			//{
			//	// we do not really know here if the sampler has been
			//	// defined in the shader.
			//	t.second->bind(t.first);
			//}
			//for(auto element : mesh->geometryBuffers)
			//{
			//	mesh->bindIAVertexBuffer(element.first);
			//}
			//mesh->txBuffer->bind(work.first->getMaterial());
		}
	}
	drawList2.clear();

	SetResourceTransitionBarrier(m_pCommandList3.Get(),
								 m_pRenderTargets[backBufferIndex].Get(),
								 D3D12_RESOURCE_STATE_RENDER_TARGET,
								 D3D12_RESOURCE_STATE_PRESENT);

	m_pCommandList3->Close();

	ID3D12CommandList* listsToExecute[] = {m_pCommandList3.Get()};
	m_pCommandQueue->ExecuteCommandLists(ARRAYSIZE(listsToExecute), listsToExecute);

	/*Update();
	UINT backBufferIndex = m_pSwapChain4->GetCurrentBackBufferIndex();
	m_pCommandAllocator->Reset();
	m_pCommandList3->Reset(m_pCommandAllocator, m_pPipeLineState);

	ID3D12DescriptorHeap* descriptorHeaps[] = {m_pCBVHeap[backBufferIndex]};

	m_pCommandList3->SetDescriptorHeaps(ARRAYSIZE(descriptorHeaps), descriptorHeaps);

	m_pCommandList3->SetGraphicsRootSignature(m_pRootSignature);

	m_pCommandList3->SetGraphicsRootDescriptorTable(
		0, m_pCBVHeap[backBufferIndex]->GetGPUDescriptorHandleForHeapStart());

	m_pCommandList3->RSSetViewports(1, &m_Viewport);
	m_pCommandList3->RSSetScissorRects(1, &m_ScissorRect);

	SetResourceTransitionBarrier(m_pCommandList3,
								 m_pRenderTargets[backBufferIndex],
								 D3D12_RESOURCE_STATE_PRESENT,
								 D3D12_RESOURCE_STATE_RENDER_TARGET);

	D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_pRenderTargetsHeap->GetCPUDescriptorHandleForHeapStart();
	cdh.ptr += m_RenderTargetDescriptorSize * backBufferIndex;

	m_pCommandList3->OMSetRenderTargets(1, &cdh, true, nullptr);

	float clearColor[] = {0.2f, 0.2f, 0.2f, 1.0f};
	m_pCommandList3->ClearRenderTargetView(cdh, clearColor, 0, nullptr);

	m_pCommandList3->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pCommandList3->IASetVertexBuffers(0, 1, &m_VertexBufferView);

	m_pCommandList3->DrawInstanced(3, 1, 0, 0);

	SetResourceTransitionBarrier(m_pCommandList3,
								 m_pRenderTargets[backBufferIndex],
								 D3D12_RESOURCE_STATE_RENDER_TARGET,
								 D3D12_RESOURCE_STATE_PRESENT);

	m_pCommandList3->Close();

	ID3D12CommandList* listsToExecute[] = {m_pCommandList3};
	m_pCommandQueue->ExecuteCommandLists(ARRAYSIZE(listsToExecute), listsToExecute);*/
}

ID3D12RootSignature* DX12Renderer::GetRootSignature() const
{
	return m_pRootSignature.Get();
}

ID3D12DescriptorHeap* DX12Renderer::GetSRVHeap() const
{
	return m_pSRVHeap.Get();
}

ID3D12Device4* DX12Renderer::GetDevice() const
{
	return m_pDevice4.Get();
}

ID3D12GraphicsCommandList3* DX12Renderer::GetCommandList() const
{
	return m_pCommandList3.Get();
}

void DX12Renderer::ExecuteCommandList()
{
	ID3D12CommandList* listsToExecute[] = {m_pCommandList3.Get()};
	m_pCommandQueue->ExecuteCommandLists(ARRAYSIZE(listsToExecute), listsToExecute);
	WaitForGPU();
}

void DX12Renderer::CreateDevice()
{
	ComPtr<ID3D12Debug> debugController;
	ComPtr<ID3D12Debug1> debugController1;

	if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
	}

	debugController->QueryInterface(IID_PPV_ARGS(&debugController1));
	debugController1->SetEnableGPUBasedValidation(true);

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
	m_pCommandQueue->SetName(L"Ica arbetsdag");

	m_pDevice4->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
									   IID_PPV_ARGS(&m_pCommandAllocator));
	m_pCommandAllocator->SetName(L"Ica lager");

	m_pDevice4->CreateCommandList(0,
								  D3D12_COMMAND_LIST_TYPE_DIRECT,
								  m_pCommandAllocator.Get(),
								  nullptr,
								  IID_PPV_ARGS(&m_pCommandList3));

	m_pCommandList3->SetName(L"Ica Anställda");
	//m_pCommandList3->Close();
}

void DX12Renderer::CreateSwapChain(int _width, int _height)
{
	IDXGIFactory5* factory = nullptr;
	CreateDXGIFactory(IID_PPV_ARGS(&factory));

	//Swap chain incoming
	DXGI_SWAP_CHAIN_DESC1 scDesc = {};
	scDesc.Width				 = _width;
	scDesc.Height				 = _height;
	scDesc.Format				 = DXGI_FORMAT_R8G8B8A8_UNORM;
	scDesc.Stereo				 = false;
	scDesc.SampleDesc.Count		 = 1;
	scDesc.SampleDesc.Quality	= 0;
	scDesc.BufferUsage			 = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scDesc.BufferCount			 = NUM_BACK_BUFFERS;
	scDesc.Scaling				 = DXGI_SCALING_NONE;
	scDesc.SwapEffect			 = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	scDesc.AlphaMode			 = DXGI_ALPHA_MODE_UNSPECIFIED;
	scDesc.Flags				 = 0;

	IDXGISwapChain1* swapChain1 = nullptr;
	ThrowIfFailed(factory->CreateSwapChainForHwnd(
		m_pCommandQueue.Get(), GetActiveWindow(), &scDesc, nullptr, nullptr, &swapChain1));

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

	m_RenderTargetDescriptorSize =
		m_pDevice4->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_pRenderTargetsHeap->GetCPUDescriptorHandleForHeapStart();

	for(UINT i = 0; i < NUM_BACK_BUFFERS; i++)
	{
		ThrowIfFailed(m_pSwapChain4->GetBuffer(i, IID_PPV_ARGS(&m_pRenderTargets[i])));
		m_pDevice4->CreateRenderTargetView(m_pRenderTargets[i].Get(), nullptr, cdh);
		cdh.ptr += m_RenderTargetDescriptorSize;
	}
}

void DX12Renderer::SetViewportAndScissorRect(int _width, int _height)
{
	m_Viewport.TopLeftX = 0.0f;
	m_Viewport.TopLeftY = 0.0f;
	m_Viewport.Width	= (float)_width;
	m_Viewport.Height   = (float)_height;
	m_Viewport.MinDepth = 0.0f;
	m_Viewport.MaxDepth = 1.0f;

	m_ScissorRect.left   = (long)0;
	m_ScissorRect.right  = (long)_width;
	m_ScissorRect.top	= (long)0;
	m_ScissorRect.bottom = (long)_height;
}

void DX12Renderer::CreateRootSignature()
{
	/*D3D12_DESCRIPTOR_RANGE dtRanges[1];
	dtRanges[0].RangeType						  = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	dtRanges[0].NumDescriptors					  = 1;
	dtRanges[0].BaseShaderRegister				  = 0;
	dtRanges[0].RegisterSpace					  = 0;
	dtRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_DESCRIPTOR_TABLE dt;
	dt.NumDescriptorRanges = ARRAYSIZE(dtRanges);
	dt.pDescriptorRanges   = dtRanges;

	D3D12_ROOT_PARAMETER rootParam[1];
	rootParam[0].ParameterType	= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[0].DescriptorTable  = dt;
	rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	D3D12_ROOT_SIGNATURE_DESC rsDesc;
	rsDesc.Flags			 = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsDesc.NumParameters	 = ARRAYSIZE(rootParam);
	rsDesc.pParameters		 = rootParam;
	rsDesc.NumStaticSamplers = 0;
	rsDesc.pStaticSamplers   = nullptr;
*/

	//define descriptor range(s)
	D3D12_DESCRIPTOR_RANGE dtRanges[2];
	dtRanges[0].RangeType						  = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	dtRanges[0].NumDescriptors					  = 3; //only one CB in this example
	dtRanges[0].BaseShaderRegister				  = 0; //register b0
	dtRanges[0].RegisterSpace					  = 0; //register(b0,space0);
	dtRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	dtRanges[1].RangeType						  = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	dtRanges[1].NumDescriptors					  = 1; //only one CB in this example
	dtRanges[1].BaseShaderRegister				  = 3; //register t3
	dtRanges[1].RegisterSpace					  = 0; //register(b0,space1);
	dtRanges[1].OffsetInDescriptorsFromTableStart = 5;

	//create a descriptor table
	D3D12_ROOT_DESCRIPTOR_TABLE dt;
	dt.NumDescriptorRanges = ARRAYSIZE(dtRanges);
	dt.pDescriptorRanges   = dtRanges;

	//create root parameter
	D3D12_ROOT_PARAMETER rootParam[3];
	rootParam[0].ParameterType	= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[0].DescriptorTable  = dt;
	rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[1].ParameterType			   = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParam[1].Descriptor.RegisterSpace  = 0;
	rootParam[1].Descriptor.ShaderRegister = TRANSLATION;
	rootParam[1].ShaderVisibility		   = D3D12_SHADER_VISIBILITY_VERTEX;

	rootParam[2].ParameterType			   = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParam[2].Descriptor.RegisterSpace  = 0;
	rootParam[2].Descriptor.ShaderRegister = DIFFUSE_TINT;
	rootParam[2].ShaderVisibility		   = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter					  = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	sampler.AddressU				  = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.AddressV				  = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.AddressW				  = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.MipLODBias				  = 0;
	sampler.MaxAnisotropy			  = 0;
	sampler.ComparisonFunc			  = D3D12_COMPARISON_FUNC_NEVER;
	sampler.BorderColor				  = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler.MinLOD					  = 0.0f;
	sampler.MaxLOD					  = D3D12_FLOAT32_MAX;
	sampler.ShaderRegister			  = 0;
	sampler.RegisterSpace			  = 0;
	sampler.ShaderVisibility		  = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC rsDesc;
	rsDesc.Flags			 = D3D12_ROOT_SIGNATURE_FLAG_NONE;
	rsDesc.NumParameters	 = ARRAYSIZE(rootParam);
	rsDesc.pParameters		 = rootParam;
	rsDesc.NumStaticSamplers = 1;
	rsDesc.pStaticSamplers   = &sampler;

	ID3DBlob* sBlob = nullptr;
	ThrowIfFailed(
		D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sBlob, nullptr));

	ThrowIfFailed(m_pDevice4->CreateRootSignature(
		0, sBlob->GetBufferPointer(), sBlob->GetBufferSize(), IID_PPV_ARGS(&m_pRootSignature)));
}

void DX12Renderer::WaitForGPU()
{
	const UINT64 fence = m_fenceValue;
	m_pCommandQueue->Signal(m_pFence.Get(), fence);

	if(m_pFence->GetCompletedValue() < fence)
	{
		m_pFence->SetEventOnCompletion(fence, m_EventHandle);
		WaitForSingleObject(m_EventHandle, INFINITE);
	}

	m_fenceValue++;
}

void DX12Renderer::SetResourceTransitionBarrier(ID3D12GraphicsCommandList* commandList,
												ID3D12Resource* resource,
												D3D12_RESOURCE_STATES StateBefore,
												D3D12_RESOURCE_STATES StateAfter)
{
	D3D12_RESOURCE_BARRIER barrierDesc = {};

	barrierDesc.Type				   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrierDesc.Transition.pResource   = resource;
	barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrierDesc.Transition.StateBefore = StateBefore;
	barrierDesc.Transition.StateAfter  = StateAfter;

	commandList->ResourceBarrier(1, &barrierDesc);
}

////////////////////////////////////////////////////
Material* DX12Renderer::makeMaterial(const std::string& _name)
{
	return new MaterialDX12(this);
}

////////////////////////////////////////////////////
Mesh* DX12Renderer::makeMesh()
{
	return new MeshDX12();
}

////////////////////////////////////////////////////
VertexBuffer* DX12Renderer::makeVertexBuffer(size_t _size, VertexBuffer::DATA_USAGE _usage)
{
	return new VertexBufferDX12(this, _size, _usage);
}

////////////////////////////////////////////////////
Texture2D* DX12Renderer::makeTexture2D()
{
	return new Texture2DDX12(this);
}


////////////////////////////////////////////////////
Sampler2D* DX12Renderer::makeSampler2D()
{
	return new Sampler2DDX12();
}

////////////////////////////////////////////////////
RenderState* DX12Renderer::makeRenderState()
{
	return new RenderStateDX12();
}

////////////////////////////////////////////////////
std::string DX12Renderer::getShaderPath()
{
	return "..\\assets\\DX12\\";
}

////////////////////////////////////////////////////
std::string DX12Renderer::getShaderExtension()
{
	return ".hlsl";
}

////////////////////////////////////////////////////
ConstantBuffer* DX12Renderer::makeConstantBuffer(std::string _name, unsigned int _location)
{
	return new ConstantBufferDX12(this, _name, _location);
}

////////////////////////////////////////////////////
Technique* DX12Renderer::makeTechnique(Material* _material, RenderState* _renderState)
{
	return new TechniqueDX12(this, _material, _renderState);
}
