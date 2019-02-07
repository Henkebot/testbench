#pragma once
#include "../ConstantBuffer.h"
#include "DX12Common.h"
#include "DX12Renderer.h"
class ConstantBufferDX12 : public ConstantBuffer
{
public:
	ConstantBufferDX12(DX12Renderer* _render, std::string _name, unsigned int _location);
	~ConstantBufferDX12() = default;

	void setData(const void* data, size_t size, Material* m, unsigned int location);
	void _createCBV(const size_t& size);
	void bind(Material*) override;

public:
	ID3D12Resource* GetConstantBufferResc() const;

private:
	UINT m_Location;
	DX12Renderer* m_pRender;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pConstantBufferResource;
};