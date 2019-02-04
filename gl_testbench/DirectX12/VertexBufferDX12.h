#pragma once
#include "../VertexBuffer.h"
#include "DX12Renderer.h"

class VertexBufferDX12 : public VertexBuffer
{

public:
	VertexBufferDX12(ID3D12Device4* _device, size_t _size, VertexBuffer::DATA_USAGE _usage);

	void setData(const void* data, size_t size, size_t offset);
	void bind(size_t offset, size_t size, unsigned int location);
	void unbind();
	size_t getSize();

public:
	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const;
	ID3D12DescriptorHeap* GetHeap() const;
	ID3D12Resource1* GetVertexBufferResource() const;

private:
	ID3D12Device4* m_pDevice;
	size_t m_totalSize;
	ID3D12DescriptorHeap* srvHeap = nullptr;
	ID3D12Resource1* m_pVertexBufferResource;
};