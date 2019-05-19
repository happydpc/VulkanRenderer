#pragma once

#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.h>


#include "rendering/Model.h"
#include "rendering/Renderer.h"
#include "rendering/Texture.h"

#include "core/CoreTools.h"
#include "resources/Texture.h"
#include "util/Gradient.h"
#include "util/MemoryPool.h"

#include "gui/InternalGraph.h"


const int NumCells = 64;
const int vertCount = (NumCells + 1) * (NumCells + 1);
const int indCount = NumCells * NumCells * 6;
const int vertElementCount = 8;

struct TerrainCoordinateData
{
	glm::vec2 pos;
	glm::vec2 size;
	glm::i32vec2 noisePos;
	glm::vec2 noiseSize;
	int sourceImageResolution;
	glm::ivec2 gridPos;
	TerrainCoordinateData (
	    glm::vec2 pos, glm::vec2 size, glm::i32vec2 noisePos, glm::vec2 noiseSize, int imageRes, glm::ivec2 gridPos)
	: pos (pos), size (size), noisePos (noisePos), noiseSize (noiseSize), sourceImageResolution (imageRes), gridPos (gridPos)
	{
	}
};

class TerrainChunkBuffer;
class Terrain;

struct HeightMapBound
{
	glm::vec2 pos_tl;
	glm::vec2 pos_br;
	glm::vec2 uv_tl = glm::vec2 (0.0, 0.0);
	glm::vec2 uv_br = glm::vec2 (1.0, 1.0);
	float height_scale = 1;
	float width_scale = 1;
};

struct TerrainQuad
{
	TerrainQuad (
	    glm::vec2 pos,
	    glm::vec2 size,
	    glm::i32vec2 logicalPos,
	    glm::i32vec2 logicalSize,
	    int level,
	    glm::i32vec2 subDivPos,
	    float centerHeightValue,
	    Terrain* terrain);

	static float GetUVvalueFromLocalIndex (float i, int numCells, int level, int subDivPos);

	// Create a mesh chunk for rendering using fastgraph as the input data
	void GenerateTerrainChunk (InternalGraph::GraphUser& graphUser, float heightScale, float widthScale);

	glm::vec2 pos;            // position of corner
	glm::vec2 size;           // width and length
	glm::i32vec2 logicalPos;  // where in the proc-gen space it is (for noise images)
	glm::i32vec2 logicalSize; // how wide the area is.
	glm::i32vec2 subDivPos;   // where in the subdivision grid it is (for splatmap)
	int level = 0;            // how deep the quad is
	float heightValAtCenter = 0;
	bool isSubdivided = false;

	Terrain* terrain; // who owns it
	int index = -1; // index into chunkBuffer

	HeightMapBound bound;

	// index into terrain's quadMap
	struct SubQuads
	{
		int UpLeft = -1;
		int DownLeft = -1;
		int UpRight = -1;
		int DownRight = -1;
	} subQuads;
};

class Terrain
{
	public:
	
	std::unordered_map<int, TerrainQuad> quadMap;

	int rootQuad = 0;

	int maxLevels;
	int maxNumQuads;
	int numQuads = 1;

	TerrainCoordinateData coordinateData;
	float heightScale = 100;

	VulkanRenderer& renderer;

	std::unique_ptr<Pipeline> normal;
	std::unique_ptr<Pipeline> wireframe;

	std::shared_ptr<VulkanDescriptor> descriptor;

	DescriptorSet descriptorSet;

	std::vector<float>* heightMapData;
	std::shared_ptr<VulkanTexture> heightMapTexture;

	std::byte* splatMapData;
	int splatMapSize;
	std::shared_ptr<VulkanTexture> terrainVulkanSplatMap;

	std::shared_ptr<VulkanBufferUniform> uniformBuffer;


	VulkanModel* terrainGrid;


	InternalGraph::GraphUser fastGraphUser;

	Gradient splatmapTextureGradient;

	SimpleTimer drawTimer;

	Terrain (VulkanRenderer& renderer,
	    InternalGraph::GraphPrototype& protoGraph,
	    int numCells,
	    int maxLevels,
	    float heightScale,
	    TerrainCoordinateData coordinateData,
	    VulkanModel* terrainGrid);

	void InitTerrain (glm::vec3 cameraPos,
	    std::shared_ptr<VulkanTexture> terrainVulkanTextureArrayAlbedo,
	    std::shared_ptr<VulkanTexture> terrainVulkanTextureArrayRoughness,
	    std::shared_ptr<VulkanTexture> terrainVulkanTextureArrayMetallic,
	    std::shared_ptr<VulkanTexture> terrainVulkanTextureArrayNormal);

	void UpdateTerrain (glm::vec3 viewerPos);

	void DrawTerrainRecursive (int quad, VkCommandBuffer cmdBuf, bool ifWireframe);
	void DrawTerrainGrid (VkCommandBuffer cmdBuf, bool ifWireframe);
	// std::vector<RGBA_pixel>* LoadSplatMapFromGenerator();

	float GetHeightAtLocation (float x, float z);

	private:
	int curEmptyIndex = 0;
	int FindEmptyIndex ();

	void InitTerrainQuad (int quad, glm::vec3 viewerPos);

	void UpdateTerrainQuad (int quad, glm::vec3 viewerPos);

	void SetupUniformBuffer ();
	void SetupImage ();
	void SetupPipeline ();

	void SetupDescriptorSets (std::shared_ptr<VulkanTexture> terrainVulkanTextureArrayAlbedo,
	    std::shared_ptr<VulkanTexture> terrainVulkanTextureArrayRoughness,
	    std::shared_ptr<VulkanTexture> terrainVulkanTextureArrayMetallic,
	    std::shared_ptr<VulkanTexture> terrainVulkanTextureArrayNormal);

	void SubdivideTerrain (int quad, glm::vec3 viewerPos);
	void UnSubdivide (int quad);

	//void PopulateQuadOffsets (int quad, std::vector<VkDeviceSize>& vert, std::vector<VkDeviceSize>& ind);

};
