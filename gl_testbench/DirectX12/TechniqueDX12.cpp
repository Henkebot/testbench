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

	gpsd.NumRenderTargets = 1;
	gpsd.RTVFormats[0]	= DXGI_FORMAT_R8G8B8A8_UNORM;
	gpsd.DSVFormat		  = DXGI_FORMAT_D32_FLOAT;

	gpsd.SampleDesc.Count = 1;
	gpsd.SampleMask		  = UINT_MAX;

	gpsd.RasterizerState.FillMode =
		rs->IsWireframe() ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;

	gpsd.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	gpsd.BlendState				  = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsd.DepthStencilState		  = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	gpsd.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
	ThrowIfFailed(
		ren->GetDevice()->CreateGraphicsPipelineState(&gpsd, IID_PPV_ARGS(&m_pPipelineState)));
}

void TechniqueDX12::enable(Renderer* renderer)
{
	DX12Renderer* ren = dynamic_cast<DX12Renderer*>(renderer);

	ren->GetCommandList()->SetPipelineState(m_pPipelineState.Get());

	material->enable();
}
