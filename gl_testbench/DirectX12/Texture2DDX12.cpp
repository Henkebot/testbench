#include "Texture2DDX12.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Texture2DDX12::Texture2DDX12(DX12Renderer* _render)
	: m_pRender(_render)
{}

int Texture2DDX12::loadFromFile(std::string filename)
{

	int w, h, bpp;
	stbi_uc* rgb = stbi_load(filename.c_str(), &w, &h, &bpp, STBI_rgb_alpha);
	if(rgb == nullptr)
	{
		fprintf(stderr, "Error loading texture file: %s\n", filename.c_str());
		return -1;
	}
	//TODO(Henrik): Check BPP

	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.MipLevels			= 1;
	textureDesc.Format				= DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.Width				= w;
	textureDesc.Height				= h;
	textureDesc.Flags				= D3D12_RESOURCE_FLAG_NONE;
	textureDesc.DepthOrArraySize	= 1;
	textureDesc.SampleDesc.Count	= 1;
	textureDesc.SampleDesc.Quality  = 0;
	textureDesc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	CD3DX12_HEAP_PROPERTIES heapProp(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(
		m_pRender->GetDevice()->CreateCommittedResource(&heapProp,
														D3D12_HEAP_FLAG_NONE,
														&textureDesc,
														D3D12_RESOURCE_STATE_COPY_DEST,
														nullptr,
														IID_PPV_ARGS(&m_pTextureResource)));

	// Get the size needed for this texture buffer
	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_pTextureResource.Get(), 0, 1);

	// This is the GPU upload buffer.
	CD3DX12_HEAP_PROPERTIES heapProp2(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC rescDesc(CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize));
	ThrowIfFailed(m_pRender->GetDevice()->CreateCommittedResource(&heapProp2,
																  D3D12_HEAP_FLAG_NONE,
																  &rescDesc,
																  D3D12_RESOURCE_STATE_GENERIC_READ,
																  nullptr,
																  IID_PPV_ARGS(&m_pTextureUpload)));

	// Now do we copy the data to the heap we created in this scope and then schedule a copy
	// from this "upload heap" to the real texture 2d?

	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData				   = &rgb[0];
	textureData.RowPitch			   = w * bpp;
	textureData.SlicePitch			   = textureData.RowPitch * w;

	UpdateSubresources(
		m_pRender->GetCommandList(), m_pTextureResource.Get(), m_pTextureUpload.Get(), 0, 0, 1, &textureData);

	CD3DX12_RESOURCE_BARRIER transition(
		CD3DX12_RESOURCE_BARRIER::Transition(m_pTextureResource.Get(),
											 D3D12_RESOURCE_STATE_COPY_DEST,
											 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	m_pRender->GetCommandList()->ResourceBarrier(1, &transition);

	// Describe and create a SRV for the texture.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format							= textureDesc.Format;
	srvDesc.ViewDimension					= D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels				= 1;


	UINT descriptorSize = m_pRender->GetDevice()->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(
		m_pRender->GetSRVHeap()->GetCPUDescriptorHandleForHeapStart(), 5 /*Heap slot*/, descriptorSize);

	m_pRender->GetDevice()->CreateShaderResourceView(
		m_pTextureResource.Get(), &srvDesc, handle);

	m_pRender->GetCommandList()->Close();

	m_pRender->ExecuteCommandList();
	stbi_image_free(rgb);


}

void Texture2DDX12::bind(unsigned int slot) {}
