#pragma once

#include "../Material.h"

class DX12Renderer;

class MaterialDX12 : public Material
{
public:
	MaterialDX12();
	~MaterialDX12() = default;

	// set shader name, DOES NOT COMPILE
	void setShader(const std::string& _shaderFileName, ShaderType _type) override;

	// removes any resource linked to shader type
	void removeShader(ShaderType _type) override;

	void setDiffuse(Color _c) override;

	/*
	 * Compile and link all shaders
	 * Returns 0  if compilation/linking succeeded.
	 * Returns -1 if compilation/linking fails.
	 * Error is returned in errString
	 * A Vertex and a Fragment shader MUST be defined.
	 * If compileMaterial is called again, it should RE-COMPILE the shader
	 * In principle, it should only be necessary to re-compile if the defines set 
	 * has changed.
	*/
	virtual int compileMaterial(std::string& _errString) override;

	// this constant buffer will be bound every time we bind the material
	virtual void addConstantBuffer(std::string name, unsigned int location) override;

	// location identifies the constant buffer in a unique way
	virtual void
	updateConstantBuffer(const void* data, size_t size, unsigned int location) override;

	// activate the material for use.
	virtual int enable() override;

	// disable material
	virtual void disable() override;
};