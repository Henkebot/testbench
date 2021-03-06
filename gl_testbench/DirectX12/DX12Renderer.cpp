#include "DX12Renderer.h"
#include "ConstantBufferDX12.h"
#include "MaterialDX12.h"
#include "MeshDX12.h"
#include "RenderStateDX12.h"
#include "TechniqueDX12.h"
#include "VertexBufferDX12.h"

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
	CreateRootSignature();

	// TODO(Henrik): Remove these
	//CreateShadersAndPipeLineState();
	//CreateConstantBufferResources();
	//CreateTriangleData();

	WaitForGPU();

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
	return 0;
}

////////////////////////////////////////////////////
void DX12Renderer::setClearColor(float _r, float _g, float _b, float _a) {}

////////////////////////////////////////////////////
void DX12Renderer::clearBuffer(unsigned int _mask) {}

////////////////////////////////////////////////////
void DX12Renderer::setRenderState(RenderState* _renderState)
{
	// Here do we enable the renderstate which contains the pipeline state and root signature
}

////////////////////////////////////////////////////
void DX12Renderer::submit(Mesh* _mesh)
{
	WaitForGPU();
	drawList2[_mesh->technique].push_back(_mesh);
}

////////////////////////////////////////////////////
void DX12Renderer::frame()
{

	ThrowIfFailed(m_pCommandAllocator->Reset());
	ThrowIfFailed(m_pCommandList3->Reset(m_pCommandAllocator, nullptr));

	m_pCommandList3->SetGraphicsRootSignature(m_pRootSignature);

	m_pCommandList3->RSSetViewports(1, &m_Viewport);
	m_pCommandList3->RSSetScissorRects(1, &m_ScissorRect);

	m_pCommandList3->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_frameIndex],
											  D3D12_RESOURCE_STATE_PRESENT,
											  D3D12_RESOURCE_STATE_RENDER_TARGET));

	D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_pRenderTargetsHeap->GetCPUDescriptorHandleForHeapStart();
	cdh.ptr += m_RenderTargetDescriptorSize * m_frameIndex;

	m_pCommandList3->OMSetRenderTargets(1, &cdh, true, nullptr);

	static float clearColor[] = {0.2f, 0.2f, 0.2f, 1.0f};
	clearColor[0] += 0.001f;
	m_pCommandList3->ClearRenderTargetView(cdh, clearColor, 0, nullptr);

	m_pCommandList3->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for(auto work : drawList2)
	{
		// Sets the pipeline state
		work.first->enable(this);

		for(auto mesh : work.second)
		{

			for(auto& buffer : mesh->geometryBuffers)
			{
				VertexBufferDX12* vb = dynamic_cast<VertexBufferDX12*>(buffer.second.buffer);
				UINT startSlot		 = buffer.first;

				m_pCommandList3->IASetVertexBuffers(startSlot, 1, &vb->m_VertexBufferView);
			}

			//for(auto t : mesh->textures)
			//{
			//	// we do not really know here if the sampler has been
			//	// defined in the shader.
			//	t.second->bind(t.first);
			//}
			//
			//mesh->txBuffer->bind(work.first->getMaterial());
			m_pCommandList3->DrawInstanced(3, 1, 0, 0);
		}
	}
	drawList2.clear();

	/*//m_pCommandList3->SetDescriptorHeaps(ARRAYSIZE(descriptorHeaps), descriptorHeaps);

	//m_pCommandList3->SetGraphicsRootSignature(m_pRootSignature);

	m_pCommandList3->SetGraphicsRootDescriptorTable(
		0, m_pCBVHeap[backBufferIndex]->GetGPUDescriptorHandleForHeapStart());

	m_pCommandList3->RSSetViewports(1, &m_Viewport);
	m_pCommandList3->RSSetScissorRects(1, &m_ScissorRect);

	SetResourceTransitionBarrier(m_pCommandList3,
								 m_pRenderTargets[backBufferIndex],
								 D3D12_RESOURCE_STATE_PRESENT,
								 D3D12_RESOURCE_STATE_RENDER_TARGET);


	m_pCommandList3->IASetVertexBuffers(0, 1, &m_VertexBufferView);
	*/

	m_pCommandList3->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_frameIndex],
											  D3D12_RESOURCE_STATE_RENDER_TARGET,
											  D3D12_RESOURCE_STATE_PRESENT));

	m_pCommandList3->Close();

	ID3D12CommandList* listsToExecute[] = {m_pCommandList3};
	m_pCommandQueue->ExecuteCommandLists(_countof(listsToExecute), listsToExecute);
}

void DX12Renderer::CreateDevice()
{
	ComPtr<ID3D12Debug> debugController;
	UINT dxgiFactoryFlags = 0;
	if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
	ComPtr<ID3D12Debug1> debugcontroller1;
	debugController->QueryInterface(IID_PPV_ARGS(&debugcontroller1));
	debugcontroller1->SetEnableGPUBasedValidation(true);


	IDXGIFactory4* factory = nullptr;
	IDXGIAdapter1* adapter = nullptr;

	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));
	//Loop through and find adapter
	for(UINT adapterIndex = 0;
		DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(adapterIndex, &adapter);
		++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);
		printf("%ls\n", desc.Description);

		if(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			continue;

		if(SUCCEEDED(
			   D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)))
		{
			break;
		}

		SafeRelease(&adapter);
		adapter = nullptr;
	}

	if(adapter)
	{
		HRESULT hr = S_OK;
		ThrowIfFailed(
			hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_pDevice4)));

		SafeRelease(&adapter);
	}
	else
	{
		ThrowIfFailed(
			D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_pDevice4)));
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
		m_pCommandQueue, GetActiveWindow(), &scDesc, nullptr, nullptr, &swapChain1));

	ThrowIfFailed(swapChain1->QueryInterface(IID_PPV_ARGS(&m_pSwapChain4)));

	m_frameIndex = m_pSwapChain4->GetCurrentBackBufferIndex();

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
	dhd.NumDescriptors			   = NUM_BACK_BUFFERS;
	dhd.Type					   = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	ThrowIfFailed(m_pDevice4->CreateDescriptorHeap(&dhd, IID_PPV_ARGS(&m_pRenderTargetsHeap)));

	m_RenderTargetDescriptorSize =
		m_pDevice4->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_pRenderTargetsHeap->GetCPUDescriptorHandleForHeapStart();

	for(UINT i = 0; i < NUM_BACK_BUFFERS; i++)
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
	D3D12_ROOT_PARAMETER rootParam[4];
	{
		//POSITION
		rootParam[0].ParameterType			   = D3D12_ROOT_PARAMETER_TYPE_SRV;
		rootParam[0].Descriptor.RegisterSpace  = 0;
		rootParam[0].Descriptor.ShaderRegister = POSITION;
		rootParam[0].ShaderVisibility		   = D3D12_SHADER_VISIBILITY_VERTEX;

		//NORMAL
		rootParam[1].ParameterType			   = D3D12_ROOT_PARAMETER_TYPE_SRV;
		rootParam[1].Descriptor.RegisterSpace  = 0;
		rootParam[1].Descriptor.ShaderRegister = NORMAL;
		rootParam[1].ShaderVisibility		   = D3D12_SHADER_VISIBILITY_VERTEX;

		//TEXTURE COORDINATE
		rootParam[2].ParameterType			   = D3D12_ROOT_PARAMETER_TYPE_SRV;
		rootParam[2].Descriptor.RegisterSpace  = 0;
		rootParam[2].Descriptor.ShaderRegister = TEXTCOORD;
		rootParam[2].ShaderVisibility		   = D3D12_SHADER_VISIBILITY_VERTEX;

		//TRANSLATION
		rootParam[3].ParameterType			   = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParam[3].Descriptor.RegisterSpace  = 0;
		rootParam[3].Descriptor.ShaderRegister = TRANSLATION;
		rootParam[3].ShaderVisibility		   = D3D12_SHADER_VISIBILITY_VERTEX;

		//TODO(Henrik): Diffuse Tint and slot?
	}

	D3D12_ROOT_SIGNATURE_DESC rsDesc;
	rsDesc.NumParameters	 = ARRAYSIZE(rootParam);
	rsDesc.pParameters		 = rootParam;
	rsDesc.NumStaticSamplers = 0;
	rsDesc.pStaticSamplers   = nullptr;
	rsDesc.Flags			 = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	ID3DBlob* sBlob;
	ThrowIfFailed(
		D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sBlob, nullptr));

	ThrowIfFailed(m_pDevice4->CreateRootSignature(
		0, sBlob->GetBufferPointer(), sBlob->GetBufferSize(), IID_PPV_ARGS(&m_pRootSignature)));
	m_pRootSignature->SetName(L"Main Root signature");
}

void DX12Renderer::CreateShadersAndPipeLineState()
{
	ID3DBlob* vertexBlob;
	D3DCompileFromFile(
		L"VertexShader.hlsl", nullptr, nullptr, "VS_main", "vs_5_0", 0, 0, &vertexBlob, nullptr);

	ID3DBlob* pixelBlob;
	D3DCompileFromFile(
		L"PixelShader.hlsl", nullptr, nullptr, "PS_main", "ps_5_0", 0, 0, &pixelBlob, nullptr);

	D3D12_INPUT_ELEMENT_DESC inputElementDesc[] = {{"POSITION",
													0,
													DXGI_FORMAT_R32G32B32_FLOAT,
													0,
													0,
													D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
													0},
												   {"COLOR",
													0,
													DXGI_FORMAT_R32G32B32_FLOAT,
													0,
													12,
													D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
													0}};

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
	inputLayoutDesc.pInputElementDescs = inputElementDesc;
	inputLayoutDesc.NumElements		   = ARRAYSIZE(inputElementDesc);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsd = {};
	gpsd.pRootSignature						= m_pRootSignature;
	gpsd.InputLayout						= inputLayoutDesc;
	gpsd.PrimitiveTopologyType				= D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	gpsd.VS.pShaderBytecode					= vertexBlob->GetBufferPointer();
	gpsd.VS.BytecodeLength					= vertexBlob->GetBufferSize();
	gpsd.PS.pShaderBytecode					= pixelBlob->GetBufferPointer();
	gpsd.PS.BytecodeLength					= pixelBlob->GetBufferSize();

	gpsd.RTVFormats[0]	= DXGI_FORMAT_R8G8B8A8_UNORM;
	gpsd.NumRenderTargets = 1;

	gpsd.SampleDesc.Count = 1;
	gpsd.SampleMask		  = UINT_MAX;

	gpsd.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpsd.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;

	D3D12_RENDER_TARGET_BLEND_DESC defaultRTdesc = {false,
													false,
													D3D12_BLEND_ONE,
													D3D12_BLEND_ZERO,
													D3D12_BLEND_OP_ADD,
													D3D12_BLEND_ONE,
													D3D12_BLEND_ZERO,
													D3D12_BLEND_OP_ADD,
													D3D12_LOGIC_OP_NOOP,
													D3D12_COLOR_WRITE_ENABLE_ALL};

	for(UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
		gpsd.BlendState.RenderTarget[i] = defaultRTdesc;

	//m_pDevice4->CreateGraphicsPipelineState(&gpsd, IID_PPV_ARGS(&m_pPipeLineState));
}

void DX12Renderer::CreateConstantBufferResources()
{
	for(int i = 0; i < NUM_BACK_BUFFERS; i++)
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDescriptorDesc = {};
		heapDescriptorDesc.NumDescriptors			  = 1;
		heapDescriptorDesc.Flags					  = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDescriptorDesc.Type						  = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		m_pDevice4->CreateDescriptorHeap(&heapDescriptorDesc, IID_PPV_ARGS(&m_pCBVHeap[i]));
	}

	UINT cbSizeAligned = (sizeof(m_ConstantBuffer) + 255) & ~255;

	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type					 = D3D12_HEAP_TYPE_UPLOAD;
	heapProperties.CPUPageProperty		 = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.CreationNodeMask		 = 1;
	heapProperties.VisibleNodeMask		 = 1;
	heapProperties.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension			 = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width				 = cbSizeAligned;
	resourceDesc.Height				 = 1;
	resourceDesc.DepthOrArraySize	= 1;
	resourceDesc.MipLevels			 = 1;
	resourceDesc.SampleDesc.Count	= 1;
	resourceDesc.Layout				 = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	for(int i = 0; i < NUM_BACK_BUFFERS; i++)
	{
		m_pDevice4->CreateCommittedResource(&heapProperties,
											D3D12_HEAP_FLAG_NONE,
											&resourceDesc,
											D3D12_RESOURCE_STATE_GENERIC_READ,
											nullptr,
											IID_PPV_ARGS(&m_pConstantBufferResource[i]));

		m_pConstantBufferResource[i]->SetName(L"cb heap");

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = m_pConstantBufferResource[i]->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes	= cbSizeAligned;

		m_pDevice4->CreateConstantBufferView(&cbvDesc,
											 m_pCBVHeap[i]->GetCPUDescriptorHandleForHeapStart());
	}
}

void DX12Renderer::CreateTriangleData()
{
	struct Vertex
	{
		float x, y, z; // Position
		float r, g, b; // Color
	};

	Vertex triangleVertices[3] = {
		0.0f,
		0.5f,
		0.0f, //v0 pos
		1.0f,
		0.0f,
		0.0f, //v0 color

		0.5f,
		-0.5f,
		0.0f, //v1
		0.0f,
		1.0f,
		0.0f, //v1 color

		-0.5f,
		-0.5f,
		0.0f, //v2
		0.0f,
		0.0f,
		1.0f //v2 color
	};

	D3D12_HEAP_PROPERTIES hp = {};
	hp.Type					 = D3D12_HEAP_TYPE_UPLOAD;
	hp.CreationNodeMask		 = 1;
	hp.VisibleNodeMask		 = 1;

	D3D12_RESOURCE_DESC rd = {};
	rd.Dimension		   = D3D12_RESOURCE_DIMENSION_BUFFER;
	rd.Width			   = sizeof(triangleVertices);
	rd.Height			   = 1;
	rd.DepthOrArraySize	= 1;
	rd.MipLevels		   = 1;
	rd.SampleDesc.Count	= 1;
	rd.Layout			   = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	m_pDevice4->CreateCommittedResource(&hp,
										D3D12_HEAP_FLAG_NONE,
										&rd,
										D3D12_RESOURCE_STATE_GENERIC_READ,
										nullptr,
										IID_PPV_ARGS(&m_pVertexBufferResource));

	m_pVertexBufferResource->SetName(L"vb heap");

	void* dataBegin   = nullptr;
	D3D12_RANGE range = {0, 0};
	m_pVertexBufferResource->Map(0, &range, &dataBegin);
	memcpy(dataBegin, triangleVertices, sizeof(triangleVertices));
	m_pVertexBufferResource->Unmap(0, nullptr);

	m_VertexBufferView.BufferLocation = m_pVertexBufferResource->GetGPUVirtualAddress();
	m_VertexBufferView.StrideInBytes  = sizeof(Vertex);
	m_VertexBufferView.SizeInBytes	= sizeof(triangleVertices);
}

void DX12Renderer::WaitForGPU()
{
	const UINT64 fence = m_fenceValue;
	m_pCommandQueue->Signal(m_pFence, fence);

	m_frameIndex = m_pSwapChain4->GetCurrentBackBufferIndex();

	if(m_pFence->GetCompletedValue() < fence)
	{
		m_pFence->SetEventOnCompletion(fence, m_EventHandle);
		WaitForSingleObject(m_EventHandle, INFINITE);
	}

	m_fenceValue++;
}

void DX12Renderer::Update()
{
	//Update color values in constant buffer
	for(int i = 0; i < 3; i++)
	{

		m_ConstantBufferCPU.colorChannel[i] += 0.0001f * (i + 1);
		if(m_ConstantBufferCPU.colorChannel[i] > 1)

		{
			m_ConstantBufferCPU.colorChannel[i] = 0;
		}
	}

	//Update GPU memory
	void* mappedMem		  = nullptr;
	D3D12_RANGE readRange = {0, 0}; //We do not intend to read this resource on the CPU.
	if(SUCCEEDED(m_pConstantBufferResource[m_pSwapChain4->GetCurrentBackBufferIndex()]->Map(
		   0, &readRange, &mappedMem)))
	{
		memcpy(mappedMem, &m_ConstantBufferCPU, sizeof(m_ConstantBuffer));

		D3D12_RANGE writeRange = {0, sizeof(m_ConstantBuffer)};
		m_pConstantBufferResource[m_pSwapChain4->GetCurrentBackBufferIndex()]->Unmap(0,
																					 &writeRange);
	}
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
	return new MaterialDX12(m_pDevice4);
}

////////////////////////////////////////////////////
Mesh* DX12Renderer::makeMesh()
{
	return new MeshDX12();
}

////////////////////////////////////////////////////
VertexBuffer* DX12Renderer::makeVertexBuffer(size_t _size, VertexBuffer::DATA_USAGE _usage)
{
	return new VertexBufferDX12(m_pDevice4, _size, _usage);
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
	return new ConstantBufferDX12(m_pDevice4, _name, _location);
}

////////////////////////////////////////////////////
Technique* DX12Renderer::makeTechnique(Material* _material, RenderState* _renderState)
{
	return new TechniqueDX12(m_pDevice4, m_pRootSignature, _material, _renderState);
}
