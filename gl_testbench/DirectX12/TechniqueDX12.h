#pragma once
#include "../Technique.h"
#include "DX12Common.h"

class TechniqueDX12 : public Technique
{
public:
	TechniqueDX12(Renderer* _renderer, Material* _material, RenderState* _renderstate);

	void enable(Renderer* renderer) override;

private:
	ID3D12PipelineState* m_pPipelineState;
};
