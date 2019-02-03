#pragma once

#include "../Technique.h"
#include "DX12Common.h"

class TechniqueDX12 : public Technique
{
public:
	TechniqueDX12(ID3D12Device4* _device,
				  ID3D12RootSignature* _rootsignature,
				  Material* _material,
				  RenderState* _renderState);

	void enable(Renderer* _renderer) override;

private:
	ID3D12PipelineState* m_pPipelineState;
};