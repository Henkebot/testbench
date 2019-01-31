#include "VertexBufferDX12.h"
#include "DX12Common.h"

VertexBufferDX12::VertexBufferDX12(ID3D12Device4* _device,
								   size_t _size,
								   VertexBuffer::DATA_USAGE _usage)
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

	void* dataBegin   = nullptr;
	D3D12_RANGE range = {0, 0};
	ThrowIfFailed(m_pVertexBufferResource->Map(0, &range, &dataBegin));

	memcpy(dataBegin, _data, sizeof(_size));

	m_pVertexBufferResource->Unmap(0, nullptr);

	//m_VertexBufferView.BufferLocation = m_pVertexBufferResource->GetGPUVirtualAddress();
	//m_VertexBufferView.StrideInBytes  = sizeof(Vertex);
	//m_VertexBufferView.SizeInBytes	= sizeof(_data);
}

void VertexBufferDX12::bind(size_t offset, size_t size, unsigned int location) {}

void VertexBufferDX12::unbind() {}

size_t VertexBufferDX12::getSize()
{
	return size_t();
}
