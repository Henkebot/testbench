#pragma once
#include "../RenderState.h"
#include "DX12Common.h"

class RenderStateDX12 : public RenderState
{
public:
	RenderStateDX12();
	~RenderStateDX12() = default;
	void setWireFrame(bool _wired);

	// activate all options in this render state.
	void set();

public:
	bool IsWireframe() const;

private:
	bool m_bWireframe;

	ID3D12PipelineState* m_pPipelineState;
};