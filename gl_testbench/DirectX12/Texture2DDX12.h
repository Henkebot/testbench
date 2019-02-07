#pragma once

#include "../Texture2D.h"
#include "DX12Renderer.h"

class Texture2DDX12 : public Texture2D
{
public:
	Texture2DDX12(DX12Renderer* _render);
	// returns 0 if texture was loaded.
	int loadFromFile(std::string filename) override;

	// bind texture to be used in the pipeline, binding to
	// slot "slot" in the active fragment shader.
	// slot can have different interpretation depending on the API.
	void bind(unsigned int slot) override;

private:
	unsigned char* rgb;
	ID3D12Resource* m_pTextureResource;
	ID3D12Resource* m_pTextureUpload;
	DX12Renderer* m_pRender;
};