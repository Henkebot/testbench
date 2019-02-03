#include "ConstantBufferDX12.h"
#include "MaterialDX12.h"

ConstantBufferDX12::ConstantBufferDX12(ID3D12Device4* _device,
									   std::string _name,
									   unsigned int _location)
{
	for(int i = 0; i < NUM_BACK_BUFFERS; i++)
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDescriptorDesc = {};
		heapDescriptorDesc.NumDescriptors			  = 1;
		heapDescriptorDesc.Flags					  = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDescriptorDesc.Type						  = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		ThrowIfFailed(
			_device->CreateDescriptorHeap(&heapDescriptorDesc, IID_PPV_ARGS(&m_pCBVHeap[i])));
	}
}

void ConstantBufferDX12::setData(const void* data, size_t size, Material* m, unsigned int location)
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

	MaterialDX12* mat = dynamic_cast<MaterialDX12*>(m);

	for(int i = 0; i < NUM_BACK_BUFFERS; i++)
	{
		mat->m_pDevice->CreateCommittedResource(&heapProperties,
												D3D12_HEAP_FLAG_NONE,
												&resourceDesc,
												D3D12_RESOURCE_STATE_GENERIC_READ,
												nullptr,
												IID_PPV_ARGS(&m_pConstantBufferResource[i]));

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = m_pConstantBufferResource[i]->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes	= cbSizeAligned;

		mat->m_pDevice->CreateConstantBufferView(
			&cbvDesc, m_pCBVHeap[i]->GetCPUDescriptorHandleForHeapStart());
	}

	void* mappedMem		  = nullptr;
	D3D12_RANGE readRange = {0, 0}; //We do not intend to read this resource on the CPU.
	m_pConstantBufferResource[0]->Map(0, &readRange, &mappedMem);

	memcpy(mappedMem, data, size);

	D3D12_RANGE writeRange = {0, size};
	m_pConstantBufferResource[0]->Unmap(0, &writeRange);
}

void ConstantBufferDX12::bind(Material*) {}
