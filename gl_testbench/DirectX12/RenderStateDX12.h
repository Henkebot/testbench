#pragma once
#include "../RenderState.h"

class RenderStateDX12 : public RenderState
{
public:
	RenderStateDX12();
	~RenderStateDX12();
	void setWireFrame(bool _wired);

	// activate all options in this render state.
	void set();

private:
	bool m_bWireframe;
};