#pragma once
#include "../ConstantBuffer.h"
#include "DX12Common.h"
class ConstantBufferDX12 : public ConstantBuffer
{
public:
	ConstantBufferDX12(ID3D12Device4* _device, std::string _name, unsigned int _location);
	~ConstantBufferDX12();

	void setData(const void* data, size_t size, Material* m, unsigned int location);
	void bind(Material*);

private:
	ID3D12DescriptorHeap* m_pCBVHeap[NUM_BACK_BUFFERS];
	ID3D12Resource* m_pConstantBufferResource[NUM_BACK_BUFFERS];
};