#include "TechniqueDX12.h"
#include "DX12Renderer.h"
#include "MaterialDX12.h"
#include "RenderStateDX12.h"

TechniqueDX12::TechniqueDX12(ID3D12Device4* _device,
							 ID3D12RootSignature* _rootsignature,
							 Material* _material,
							 RenderState* _renderState)
	: Technique(_material, _renderState)
{
	MaterialDX12* matdx  = dynamic_cast<MaterialDX12*>(_material);
	RenderStateDX12* rdx = dynamic_cast<RenderStateDX12*>(_renderState);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pDesc = {};

	pDesc.pRootSignature		= _rootsignature;
	pDesc.VS					= matdx->GetShaderByteDesc(Material::ShaderType::VS);
	pDesc.PS					= matdx->GetShaderByteDesc(Material::ShaderType::PS);
	pDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	pDesc.RTVFormats[0]	= DXGI_FORMAT_R8G8B8A8_UNORM;
	pDesc.NumRenderTargets = 1;

	pDesc.SampleDesc.Count = 1;
	pDesc.SampleMask	   = UINT_MAX;

	bool wireframe = rdx->IsWireframe();

	pDesc.RasterizerState.FillMode = wireframe ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
	pDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;

	//TODO(Henrik): We probably need this later
	pDesc.DepthStencilState.DepthEnable   = FALSE;
	pDesc.DepthStencilState.StencilEnable = FALSE;

	pDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	ThrowIfFailed(_device->CreateGraphicsPipelineState(&pDesc, IID_PPV_ARGS(&m_pPipelineState)));
	m_pPipelineState->SetName(L"TechniqueDX12 pipeline");
}

void TechniqueDX12::enable(Renderer* _renderer)
{
	DX12Renderer* r = dynamic_cast<DX12Renderer*>(_renderer);

	// NOTE(Henrik): Dont know if it is better to have
	// different command lists with different pipeline states
	r->m_pCommandList3->SetPipelineState(m_pPipelineState);



}
