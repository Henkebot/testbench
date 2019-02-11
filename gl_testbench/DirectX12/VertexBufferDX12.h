#pragma once
#include "../VertexBuffer.h"
#include "DX12Renderer.h"

class VertexBufferDX12 : public VertexBuffer
{

public:
	VertexBufferDX12(DX12Renderer* _renderer, size_t _size, VertexBuffer::DATA_USAGE _usage);
	~VertexBufferDX12() = default;
	void setData(const void* data, size_t size, size_t offset);
	void bind(size_t offset, size_t size, unsigned int location);
	void unbind();
	size_t getSize();

private:
	DX12Renderer* m_pRender;
	size_t m_totalSize;
	Microsoft::WRL::ComPtr < ID3D12Resource1> m_pVertexBufferUpload;
	bool m_bHasSRV;
	void _createSRV(size_t _ElementSize);
};