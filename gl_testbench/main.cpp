#include <string>
#include <SDL_keyboard.h>
#include <SDL_events.h>
#include <type_traits> 
#include <assert.h>

#include "Renderer.h"
#include "Mesh.h"
#include "Texture2D.h"
#include <math.h>

using namespace std;
Renderer* renderer;

// flat scene at the application level...we don't care about this here.
// do what ever you want in your renderer backend.
// all these objects are loosely coupled, creation and destruction is responsibility
// of the testbench, not of the container objects
vector<Mesh*> scene;
vector<Material*> materials;
vector<Technique*> techniques;
vector<Texture2D*> textures;
vector<Sampler2D*> samplers;

VertexBuffer* pos;
VertexBuffer* nor;
VertexBuffer* uvs;

// forward decls
void updateScene();
void renderScene();


constexpr float DENSITY = 0.5; // 1 is spread across circle, 0.00001 is super dense
constexpr int TOTAL_TRIS = 10;
constexpr int TOTAL_PLACES = (float)TOTAL_TRIS / DENSITY;
float xt[TOTAL_PLACES], yt[TOTAL_PLACES], zt[TOTAL_PLACES];

// lissajous points
typedef union { 
	struct { float x, y, z, w; };
	struct { float r, g, b, a; };
} float4;

typedef union { 
	struct { float x, y; };
	struct { float u, v; };
} float2;


void run() {

	SDL_Event windowEvent;
	while (true)
	{
		if (SDL_PollEvent(&windowEvent))
		{
			if (windowEvent.type == SDL_QUIT) break;
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_ESCAPE) break;
		}
		updateScene();
		renderScene();
	}
}

/*
 update positions of triangles in the screen changing a translation only
*/
void updateScene()
{
	/*
	    For each mesh in scene list, update their position 
	*/
	static int slowDown = 0;
	static int speed = 0;

	/*
	if (slowDown++ % 500 == 0)
	{

		float translation[4] = { 0.0,0.0,0.0,0.0 };
		static int shift = 0;

		float scale = 1.0;
		translation[0] = xt[(0+shift) % (TOTAL_PLACES)];
		translation[1] = yt[(0+shift) % (TOTAL_PLACES)];
		translation[2] = zt[(0+shift) % (TOTAL_PLACES)];

		Mesh* const m0 = scene[0];
		m0->txBuffer->setData( translation, sizeof(translation), m0->technique->getMaterial(), TRANSLATION);
	//	translation[2] = 0.0;


		for (int i = 1; i < scene.size(); i++)
		{
			translation[0] = xt[(i+shift) % (TOTAL_PLACES)];
			translation[1] = yt[(i+shift) % (TOTAL_PLACES)];

			// updates the buffer data (whenever the implementation decides...)
			Mesh* mn = scene[i];
			mn->txBuffer->setData( translation, sizeof(translation), mn->technique->getMaterial(), TRANSLATION);
		}
		shift+=speed;
	}
	*/
	return;
};


void renderScene()
{
	renderer->clearBuffer(CLEAR_BUFFER_FLAGS::COLOR | CLEAR_BUFFER_FLAGS::DEPTH);
	for (auto m : scene)
	{
		renderer->submit(m);
	}
	renderer->frame();
	renderer->present();
}

int initialiseTestbench()
{
	std::string definePos = "#define POSITION " + std::to_string(POSITION) + "\n";
	std::string defineNor = "#define NORMAL " + std::to_string(NORMAL) + "\n";
	std::string defineUV = "#define TEXTCOORD " + std::to_string(TEXTCOORD) + "\n";

	std::string defineTX = "#define TRANSLATION " + std::to_string(TRANSLATION) + "\n";

	std::string defineTXName = "#define TRANSLATION_NAME " + std::string(TRANSLATION_NAME) + "\n";
	
	std::string defineDiffCol = "#define DIFFUSE_TINT " + std::to_string(DIFFUSE_TINT) + "\n";
	std::string defineDiffColName = "#define DIFFUSE_TINT_NAME " + std::string(DIFFUSE_TINT_NAME) + "\n";

	std::string defineDiffuse = "#define DIFFUSE_SLOT " + std::to_string(DIFFUSE_SLOT) + "\n";

	std::vector<std::vector<std::string>> materialDefs = {
		// vertex shader, fragment shader, defines
		// shader extension must be asked to the renderer
		// these strings should be constructed from the IA.h file!!!

		{ "VertexShader", "FragmentShader", definePos + defineNor + defineUV + defineTX + 
		   defineTXName + defineDiffCol + defineDiffColName }, 

		{ "VertexShader", "FragmentShader", definePos + defineNor + defineUV + defineTX + 
		   defineTXName + defineDiffCol + defineDiffColName }, 

		{ "VertexShader", "FragmentShader", definePos + defineNor + defineUV + defineTX + 
		   defineTXName + defineDiffCol + defineDiffColName + defineDiffuse	}
	};

	float degToRad = M_PI / 180.0;
	float scale = 359.9f / (float)TOTAL_PLACES;
	for (int a = 0; a < TOTAL_PLACES; a++)
	{
		xt[a] = 0.8f * cosf(degToRad * ((float)a*scale) * 3.0);
		yt[a] = 0.8f * sinf(degToRad * ((float)a*scale) * 2.0);
		zt[a] = 0.0f;// -0.01 * a;
	};

	// triangle geometry:
	float4 triPos[3] = { { 0.0f,  0.05, 0.0f, 1.0f },{ 0.05, -0.05, 0.0f, 1.0f },{ -0.05, -0.05, 0.0f, 1.0f } };
	float4 triNor[3] = { { 0.0f,  0.0f, 1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f, 0.0f } };
	float2 triUV[3] =  { { 0.5f,  -0.99f },{ 1.49f, 1.1f },{ -0.51, 1.1f } };

	// load Materials.
	std::string shaderPath = renderer->getShaderPath();
	std::string shaderExtension = renderer->getShaderExtension();
	float diffuse[3][4] = {
		0.0,0.0,1.0,1.0,
		0.0,1.0,0.0,1.0,
		1.0,1.0,1.0,1.0 
	};

	for (int i = 0; i < materialDefs.size(); i++)
	{
		// set material name from text file?
		Material* m = renderer->makeMaterial();
		m->setShader(shaderPath + materialDefs[i][0] + shaderExtension, Material::ShaderType::VS);
		m->setShader(shaderPath + materialDefs[i][1] + shaderExtension, Material::ShaderType::PS);

		m->addDefine(materialDefs[i][2], Material::ShaderType::VS);
		m->addDefine(materialDefs[i][2], Material::ShaderType::PS);

		std::string err;
		m->compileMaterial(err);

		// add a constant buffer to the material, to tint every triangle using this material
		m->addConstantBuffer(DIFFUSE_TINT_NAME, DIFFUSE_TINT);
		// no need to update anymore
		// when material is bound, this buffer should be also bound for access.
		m->updateConstantBuffer(diffuse[i], 4 * sizeof(float), DIFFUSE_TINT);
		
		materials.push_back(m);
	}

	// one technique with wireframe
	RenderState* renderState1 = renderer->makeRenderState();
	renderState1->setWireFrame(true);

	// basic technique
	techniques.push_back(renderer->makeTechnique(materials[0], renderState1));
	techniques.push_back(renderer->makeTechnique(materials[1], renderer->makeRenderState()));
	techniques.push_back(renderer->makeTechnique(materials[2], renderer->makeRenderState()));

	// create texture
	Texture2D* fatboy = renderer->makeTexture2D();
	fatboy->loadFromFile("../assets/textures/fatboy.png");
	Sampler2D* sampler = renderer->makeSampler2D();
	sampler->setWrap(WRAPPING::REPEAT, WRAPPING::REPEAT);
	fatboy->sampler = sampler;

	textures.push_back(fatboy);
	samplers.push_back(sampler);

	// pre-allocate one single vertex buffer for ALL triangles
	pos = renderer->makeVertexBuffer(TOTAL_TRIS * sizeof(triPos), VertexBuffer::DATA_USAGE::STATIC);
	nor = renderer->makeVertexBuffer(TOTAL_TRIS * sizeof(triNor), VertexBuffer::DATA_USAGE::STATIC);
	uvs = renderer->makeVertexBuffer(TOTAL_TRIS * sizeof(triUV), VertexBuffer::DATA_USAGE::STATIC);

	// Create a mesh array with 3 basic vertex buffers.
	for (int i = 0; i < TOTAL_TRIS; i++) {

		Mesh* m = renderer->makeMesh();

		constexpr auto numberOfPosElements = std::extent<decltype(triPos)>::value;
		size_t offset = i * sizeof(triPos);
		pos->setData(triPos, sizeof(triPos), offset);
		m->addIAVertexBufferBinding(pos, offset, numberOfPosElements, sizeof(float4), POSITION);

		constexpr auto numberOfNorElements = std::extent<decltype(triNor)>::value;
		offset = i * sizeof(triNor);
		nor->setData(triNor, sizeof(triNor), offset);
		m->addIAVertexBufferBinding(nor, offset, numberOfNorElements, sizeof(float4), NORMAL);

		constexpr auto numberOfUVElements = std::extent<decltype(triUV)>::value;
		offset = i * sizeof(triUV);
		uvs->setData(triUV, sizeof(triUV), offset);
		m->addIAVertexBufferBinding(uvs, offset, numberOfUVElements , sizeof(float2), TEXTCOORD);

		// we can create a constant buffer outside the material, for example as part of the Mesh.
		m->txBuffer = renderer->makeConstantBuffer(std::string(TRANSLATION_NAME), TRANSLATION);
		
		if (i == TOTAL_TRIS-1) {
			m->technique = techniques[2];
			m->addTexture(textures[0], DIFFUSE_SLOT);
			
		}
		else 
			// every other triangle is wireframe (except for the last)
			m->technique = techniques[ i % 2];

		scene.push_back(m);
	}
	return 0;
}

void shutdown() {
	// shutdown.
	// delete dynamic objects
	for (auto m : materials)
	{
		delete(m);
	}
	for (auto t : techniques)
	{
		delete(t);
	}
	for (auto m : scene)
	{
		delete(m);
		//for (auto g : m->geometryBuffers)
		//{
			//delete g.second.buffer;
		//}
	};
	assert(pos->refCount() == 0);
	delete pos;
	assert(nor->refCount() == 0);
	delete nor;
	assert(uvs->refCount() == 0);
	delete uvs;
	
	for (auto s : samplers)
	{
		delete s;
	}

	for (auto t : textures)
	{
		delete t;
	}
	renderer->shutdown();
};

int main(int argc, char *argv[])
{
	renderer = Renderer::makeRenderer(Renderer::BACKEND::GL45);
	renderer->initialize(800,600);
	renderer->setWinTitle("OpenGL");
	renderer->setClearColor(0.0, 0.1, 0.1, 1.0);
	initialiseTestbench();
	run();
	shutdown();
	return 0;
};
