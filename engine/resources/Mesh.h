#pragma once

#include <unordered_map>
#include <vector>

#include "cml/cml.h"


namespace job
{
class TaskManager;
}
namespace Resource::Mesh
{

enum VertexType
{
	Vert1 = 1,
	Vert2 = 2,
	Vert3 = 3,
	Vert4 = 4,
};


class VertexDescription
{
	public:
	VertexDescription (std::vector<VertexType> layout) : layout (layout){};
	int ElementCount () const;

	std::vector<VertexType> layout; // each element in the array is a different attribute, its value (1-4) is it's size
};

static VertexDescription Vert_Pos = VertexDescription ({ Vert3 });
static VertexDescription Vert_PosNorm = VertexDescription ({ Vert3, Vert3 });
static VertexDescription Vert_PosNormUv = VertexDescription ({ Vert3, Vert3, Vert2 });
static VertexDescription Vert_PosNormUvCol = VertexDescription ({ Vert3, Vert3, Vert2, Vert4 });

struct MeshData
{
	MeshData (VertexDescription desc, std::vector<float> vertexData, std::vector<uint32_t> indexData)
	: desc (desc), vertexData (vertexData), indexData (indexData)
	{
	}

	const VertexDescription desc;
	std::vector<float> vertexData;
	std::vector<uint32_t> indexData;
};

struct DeInterleavedMeshData
{
	struct FloatBufferData
	{
		FloatBufferData (VertexType type) : type (type) {}
		VertexType type;
		std::vector<float> data;
	};

	DeInterleavedMeshData (std::vector<VertexType> bufferTypes)
	{
		for (auto& t : bufferTypes)
		{
			buffers.push_back (FloatBufferData (t));
		}
	}

	std::vector<FloatBufferData> buffers;
};

MeshData CreateFlatPlane (int dim, cml::vec3f size);

MeshData CreateCube (int dim = 1);

MeshData CreateSphere (int dim = 10);

MeshData CreateWaterPlaneSubdiv (int levels = 3, int subdivs = 3);

using MeshID = uint32_t;
class Manager
{
	public:
	Manager (job::TaskManager& task_manager);

	MeshID LoadMesh ();

	private:
	job::TaskManager& task_manager;
	std::unordered_map<MeshID, MeshData> meshes;
};



} // namespace Resource::Mesh