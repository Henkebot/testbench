#pragma once
#include "../Technique.h"
#include "DX12Common.h"

class TechniqueDX12 : public Technique
{
public:
	TechniqueDX12(Renderer* _renderer, Material* _material, RenderState* _renderstate);
	~TechniqueDX12();
	void enable(Renderer* renderer) override;

private:
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pPipelineState;
};
