#include "ConstantBufferDX12.h"
#include "MaterialDX12.h"

ConstantBufferDX12::ConstantBufferDX12(DX12Renderer* _render,
									   std::string _name,
									   unsigned int _location)
	: ConstantBuffer(_name, _location)
	, m_pRender(_render)
	, m_pConstantBufferResource(nullptr)
	, m_Location(_location)
{}

void ConstantBufferDX12::setData(const void* data, size_t size, Material* m, unsigned int location)
{
	if(m_pConstantBufferResource == nullptr)
		_createCBV(size);

	void* mappedMem		  = nullptr;
	D3D12_RANGE readRange = {0, 0}; //We do not intend to read this resource on the CPU.
	m_pConstantBufferResource->Map(0, &readRange, &mappedMem);

	memcpy(mappedMem, data, size);

	D3D12_RANGE writeRange = {0, size};
	m_pConstantBufferResource->Unmap(0, &writeRange);
}

void ConstantBufferDX12::_createCBV(const size_t& size)
{
	UINT cbSizeAligned = (size + 255) & ~255;

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

	m_pRender->GetDevice()->CreateCommittedResource(&heapProperties,
													D3D12_HEAP_FLAG_NONE,
													&resourceDesc,
													D3D12_RESOURCE_STATE_GENERIC_READ,
													nullptr,
													IID_PPV_ARGS(&m_pConstantBufferResource));

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation					= m_pConstantBufferResource->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes						= cbSizeAligned;

	UINT heapSize = m_pRender->GetDevice()->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	UINT offset = (6 - m_Location) + 3;

	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(
		m_pRender->GetSRVHeap()->GetCPUDescriptorHandleForHeapStart(), offset, heapSize);

	m_pRender->GetDevice()->CreateConstantBufferView(&cbvDesc, handle);
}

void ConstantBufferDX12::bind(Material* _mat)
{
	m_pRender->GetCommandList()->SetGraphicsRootConstantBufferView(
		1, m_pConstantBufferResource->GetGPUVirtualAddress());
}

ID3D12Resource* ConstantBufferDX12::GetConstantBufferResc() const
{
	return m_pConstantBufferResource;
}
