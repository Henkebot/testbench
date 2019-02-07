#include "TechniqueDX12.h"

#include "DX12Renderer.h"
#include "MaterialDX12.h"
#include "RenderStateDX12.h"


TechniqueDX12::TechniqueDX12(Renderer* _renderer, Material* _material, RenderState* _renderstate)
	: Technique(_material, _renderstate)
{
	MaterialDX12* mat   = dynamic_cast<MaterialDX12*>(_material);
	DX12Renderer* ren   = dynamic_cast<DX12Renderer*>(_renderer);
	RenderStateDX12* rs = dynamic_cast<RenderStateDX12*>(_renderstate);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsd = {};
	gpsd.pRootSignature						= ren->GetRootSignature();
	gpsd.PrimitiveTopologyType				= D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	gpsd.VS									= mat->GetShaderByteCode(Material::ShaderType::VS);
	gpsd.PS									= mat->GetShaderByteCode(Material::ShaderType::PS);

	gpsd.RTVFormats[0]	= DXGI_FORMAT_R8G8B8A8_UNORM;
	gpsd.NumRenderTargets = 1;

	gpsd.SampleDesc.Count = 1;
	gpsd.SampleMask		  = UINT_MAX;

	gpsd.RasterizerState.FillMode =
		rs->IsWireframe() ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;

	gpsd.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;

	D3D12_RENDER_TARGET_BLEND_DESC defaultRTdesc = {false,
													false,
													D3D12_BLEND_ONE,
													D3D12_BLEND_ZERO,
													D3D12_BLEND_OP_ADD,
													D3D12_BLEND_ONE,
													D3D12_BLEND_ZERO,
													D3D12_BLEND_OP_ADD,
													D3D12_LOGIC_OP_NOOP,
													D3D12_COLOR_WRITE_ENABLE_ALL};

	for(UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
		gpsd.BlendState.RenderTarget[i] = defaultRTdesc;

	ThrowIfFailed(
		ren->GetDevice()->CreateGraphicsPipelineState(&gpsd, IID_PPV_ARGS(&m_pPipelineState)));
}

void TechniqueDX12::enable(Renderer* renderer) 
{
	DX12Renderer* ren = dynamic_cast<DX12Renderer*>(renderer);

	ren->GetCommandList()->SetPipelineState(m_pPipelineState);

	material->enable();
}
