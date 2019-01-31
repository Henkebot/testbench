#include "RenderStateDX12.h"

RenderStateDX12::RenderStateDX12()
	: m_bWireframe(false)
{}

RenderStateDX12::~RenderStateDX12() {}

void RenderStateDX12::setWireFrame(bool _wired)
{
	m_bWireframe = _wired;
}

void RenderStateDX12::set() {}
