#pragma once

#include <vector>
#include <memory>
#include <iostream>

#include "Mesh.h"

class MeshManager
{
public:
	MeshManager();
	~MeshManager();


private:

	std::vector<std::shared_ptr<Mesh>> meshHandles;

};

