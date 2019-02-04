#include "VertexBufferDX12.h"
#include "DX12Common.h"

VertexBufferDX12::VertexBufferDX12(ID3D12Device4* _device,
								   size_t _size,
								   VertexBuffer::DATA_USAGE _usage)
	: m_totalSize(_size)
	, m_pDevice(_device)
{
	UNREFERENCED_PARAMETER(_usage);

	D3D12_HEAP_PROPERTIES hp = {};
	hp.Type					 = D3D12_HEAP_TYPE_UPLOAD;
	hp.CreationNodeMask		 = 1;
	hp.VisibleNodeMask		 = 1;

	D3D12_RESOURCE_DESC rd = {};
	rd.Dimension		   = D3D12_RESOURCE_DIMENSION_BUFFER;
	rd.Width			   = _size;
	rd.Height			   = 1;
	rd.DepthOrArraySize	= 1;
	rd.MipLevels		   = 1;
	rd.SampleDesc.Count	= 1;
	rd.Layout			   = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ThrowIfFailed(_device->CreateCommittedResource(&hp,
												   D3D12_HEAP_FLAG_NONE,
												   &rd,
												   D3D12_RESOURCE_STATE_GENERIC_READ,
												   nullptr,
												   IID_PPV_ARGS(&m_pVertexBufferResource)));
}

void VertexBufferDX12::setData(const void* _data, size_t _size, size_t offset)
{
	//TODO(Henrik): Offset the data pointer

	if(nullptr == srvHeap)
	{
		D3D12_DESCRIPTOR_HEAP_DESC srvUavHeapDesc = {};
		srvUavHeapDesc.NumDescriptors			  = 1;
		srvUavHeapDesc.Type						  = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvUavHeapDesc.Flags					  = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(m_pDevice->CreateDescriptorHeap(&srvUavHeapDesc, IID_PPV_ARGS(&srvHeap)));

		size_t stride = _size / 3;

		D3D12_SHADER_RESOURCE_VIEW_DESC pDesc = {};
		pDesc.Format						  = DXGI_FORMAT_UNKNOWN;
		pDesc.ViewDimension					  = D3D12_SRV_DIMENSION_BUFFER;
		pDesc.Shader4ComponentMapping		  = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		pDesc.Buffer.FirstElement			  = 0;
		pDesc.Buffer.NumElements			  = m_totalSize / stride;
		pDesc.Buffer.StructureByteStride	  = stride;
		pDesc.Buffer.Flags					  = D3D12_BUFFER_SRV_FLAG_NONE;

		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(
			srvHeap->GetCPUDescriptorHandleForHeapStart(),
			0,
			m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

		m_pDevice->CreateShaderResourceView(m_pVertexBufferResource, &pDesc, srvHandle);
	}

	void* dataBegin   = nullptr;

	D3D12_RANGE range = {0, 0};

	ThrowIfFailed(m_pVertexBufferResource->Map(0, &range, &dataBegin));
	
	memcpy(dataBegin, _data, sizeof(_size));

	m_pVertexBufferResource->Unmap(0, nullptr);
}

void VertexBufferDX12::bind(size_t offset, size_t size, unsigned int location) {}

void VertexBufferDX12::unbind() {}

size_t VertexBufferDX12::getSize()
{
	return size_t();
}

ID3D12DescriptorHeap* VertexBufferDX12::GetHeap() const
{
	return srvHeap;
}

ID3D12Resource1* VertexBufferDX12::GetVertexBufferResource() const
{
	return m_pVertexBufferResource;
}
