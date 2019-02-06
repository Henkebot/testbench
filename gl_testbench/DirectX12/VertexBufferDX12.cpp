#include "VertexBufferDX12.h"
#include "DX12Common.h"

VertexBufferDX12::VertexBufferDX12(DX12Renderer* _renderer,
								   size_t _size,
								   VertexBuffer::DATA_USAGE _usage)
	: m_totalSize(_size)
	, m_pRender(_renderer)
	, m_bHasSRV(false)
{
	UNREFERENCED_PARAMETER(_usage);

	D3D12_HEAP_PROPERTIES defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_HEAP_PROPERTIES uploadHeapProperties  = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC bufferDesc =
		CD3DX12_RESOURCE_DESC::Buffer(m_totalSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	D3D12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(m_totalSize);

	ThrowIfFailed(
		m_pRender->GetDevice()->CreateCommittedResource(&defaultHeapProperties,
														D3D12_HEAP_FLAG_NONE,
														&bufferDesc,
														D3D12_RESOURCE_STATE_COPY_DEST,
														nullptr,
														IID_PPV_ARGS(&m_pVertexBufferResource)));

	ThrowIfFailed(
		m_pRender->GetDevice()->CreateCommittedResource(&uploadHeapProperties,
														D3D12_HEAP_FLAG_NONE,
														&uploadBufferDesc,
														D3D12_RESOURCE_STATE_GENERIC_READ,
														nullptr,
														IID_PPV_ARGS(&m_pVertexBufferUpload)));

	

}

void VertexBufferDX12::_createSRV(size_t _ElementSize)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format							= DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension					= D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement				= 0;
	srvDesc.Buffer.NumElements				= m_totalSize / _ElementSize;
	srvDesc.Buffer.StructureByteStride		= _ElementSize / 3;
	srvDesc.Buffer.Flags					= D3D12_BUFFER_SRV_FLAG_NONE;

	static int offset = 0;
	UINT size		  = m_pRender->GetDevice()->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle0(
		m_pRender->GetSRVHeap()->GetCPUDescriptorHandleForHeapStart(), offset++, size);

	m_pRender->GetDevice()->CreateShaderResourceView(m_pVertexBufferUpload, &srvDesc, srvHandle0);
	m_bHasSRV = true;
}

void VertexBufferDX12::setData(const void* _data, size_t _size, size_t offset)
{
	if(false == m_bHasSRV)
		_createSRV(_size);

	UINT8* pVertexDataBegin;
	CD3DX12_RANGE readRange(0, 0); // We do not instend to read from this resource on the CPU.
	ThrowIfFailed(m_pVertexBufferUpload->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
	pVertexDataBegin = pVertexDataBegin + offset;
	memcpy(pVertexDataBegin, _data, _size);
	m_pVertexBufferUpload->Unmap(0, nullptr);

}

void VertexBufferDX12::bind(size_t offset, size_t size, unsigned int location) {}

void VertexBufferDX12::unbind() {}

size_t VertexBufferDX12::getSize()
{
	return size_t();
}

ID3D12Resource1* VertexBufferDX12::GetVertexBufferResource() const
{
	return m_pVertexBufferResource;
}
