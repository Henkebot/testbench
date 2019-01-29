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
	CreateRootSignature();
	CreateShadersAndPipeLineState();
	CreateConstantBufferResources();
	CreateTriangleData();

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
void DX12Renderer::frame() 
{

}

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
	scDesc.Width				 = _width;
	scDesc.Height				 = _height;
	scDesc.Format				 = DXGI_FORMAT_R8G8B8A8_UNORM;
	scDesc.Stereo				 = false;
	scDesc.SampleDesc.Count		 = 1;
	scDesc.SampleDesc.Quality	= 0;
	scDesc.BufferUsage			 = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scDesc.BufferCount			 = m_NUM_BACK_BUFFERS;
	scDesc.Scaling				 = DXGI_SCALING_NONE;
	scDesc.SwapEffect			 = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	scDesc.AlphaMode			 = DXGI_ALPHA_MODE_UNSPECIFIED;
	scDesc.Flags				 = 0;

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
	dhd.NumDescriptors			   = m_NUM_BACK_BUFFERS;
	dhd.Type					   = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

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
	D3D12_DESCRIPTOR_RANGE dtRanges[1];
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

	ID3DBlob* sBlob;
	D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sBlob, nullptr);

	m_pDevice4->CreateRootSignature(
		0, sBlob->GetBufferPointer(), sBlob->GetBufferSize(), IID_PPV_ARGS(&m_pRootSignature));
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

	m_pDevice4->CreateGraphicsPipelineState(&gpsd, IID_PPV_ARGS(&m_pPipeLineState));
}

void DX12Renderer::CreateConstantBufferResources()
{
	for(int i = 0; i < m_NUM_BACK_BUFFERS; i++)
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDescriptorDesc = {};
		heapDescriptorDesc.NumDescriptors			  = 1;
		heapDescriptorDesc.Flags					  = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDescriptorDesc.Type						  = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		m_pDevice4->CreateDescriptorHeap(&heapDescriptorDesc, IID_PPV_ARGS(&m_pCBVHeap[i]));
	}

	UINT cbSizeAligned = (sizeof(ConstantBuffer) + 255) & ~255;

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

	for(int i = 0; i < m_NUM_BACK_BUFFERS; i++)
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

	if (m_pFence->GetCompletedValue() < fence)
	{
		m_pFence->SetEventOnCompletion(fence, m_EventHandle);
		WaitForSingleObject(m_EventHandle, INFINITE);
	}
	
	m_fenceValue++;
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
