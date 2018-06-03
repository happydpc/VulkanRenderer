#include "TerrainManager.h"

#include <chrono>

#include <json.hpp>

#include "../../third-party/ImGui/imgui.h"

#include "../core/Logger.h"

constexpr auto TerrainSettingsFileName = "terrain_settings.json";

TerrainCreationData::TerrainCreationData(
	std::shared_ptr<ResourceManager> resourceMan,
	std::shared_ptr<VulkanRenderer> renderer,
	std::shared_ptr<Camera> camera,
	std::shared_ptr<VulkanTexture2DArray> terrainVulkanTextureArray,
	std::shared_ptr<MemoryPool<TerrainQuad>> pool,
	InternalGraph::GraphPrototype& protoGraph,
	int numCells, int maxLevels, int sourceImageResolution, float heightScale, TerrainCoordinateData coord) :

	resourceMan(resourceMan),
	renderer(renderer),
	camera(camera),
	terrainVulkanTextureArray(terrainVulkanTextureArray),
	pool(pool),
	protoGraph(protoGraph),
	numCells(numCells),
	maxLevels(maxLevels),
	sourceImageResolution(sourceImageResolution),
	heightScale(heightScale),
	coord(coord)
{

}

void TerrainCreationWorker(TerrainManager* man) {

	while (man->isCreatingTerrain) {
		{
			std::unique_lock<std::mutex> lock(man->workerMutex);
			man->workerConditionVariable.wait(lock);
		}
		
		while(!man->terrainCreationWork.empty()) {
			auto data = man->terrainCreationWork.pop_if();
			if(data.has_value())
			{
				auto terrain = std::make_shared<Terrain>(data->pool, data->protoGraph, data->numCells, data->maxLevels, data->heightScale, data->coord);

				std::vector<RGBA_pixel>* imgData = terrain->LoadSplatMapFromGenerator();

				terrain->terrainSplatMap = data->resourceMan->texManager.loadTextureFromRGBAPixelData(data->sourceImageResolution + 1, data->sourceImageResolution + 1, imgData);

				terrain->InitTerrain(data->renderer, data->camera->Position, data->terrainVulkanTextureArray);

				{
					std::lock_guard<std::mutex> lk(man->terrain_mutex);
					man->terrains.push_back(std::move(terrain));
				}

			}
			//break out of loop if work shouldn't be continued
			if(!man->isCreatingTerrain) 
				return;	
		}
	}

}


TerrainManager::TerrainManager(InternalGraph::GraphPrototype& protoGraph) : protoGraph(protoGraph)
{
	if (settings.maxLevels < 0) {
		maxNumQuads = 1;
	}
	else {
		maxNumQuads = 1 + 16 + 50 * settings.maxLevels; //with current quad density this is the average upper bound
											   //maxNumQuads = (int)((1.0 - glm::pow(4, maxLevels + 1)) / (-3.0)); //legitimate max number of quads
	}

	//terrainQuadPool = std::make_shared<MemoryPool<TerrainQuadData, 2 * sizeof(TerrainQuadData)>>();
}

TerrainManager::~TerrainManager()
{
	isCreatingTerrain = false;
	workerConditionVariable.notify_all();
	for (auto& thread : terrainCreationWorkers) {
		thread.join();
	}
	terrainCreationWorkers.clear();
}

void TerrainManager::ResetWorkerThreads() {
	isCreatingTerrain = false;
	workerConditionVariable.notify_all();
	for (auto& thread : terrainCreationWorkers) {
		thread.join();
	}
	terrainCreationWorkers.clear();
	
	isCreatingTerrain = true;
	for (int i = 0; i < WorkerThreads; i++) {
		terrainCreationWorkers.push_back(std::thread(TerrainCreationWorker, this));
	}
}

void TerrainManager::SetupResources(std::shared_ptr<ResourceManager> resourceMan, std::shared_ptr<VulkanRenderer> renderer) {

	for (auto& item : terrainTextureFileNames) {
		terrainTextureHandles.push_back(
			TerrainTextureNamedHandle(
				item,
				resourceMan->texManager.loadTextureFromFileRGBA("assets/Textures/TerrainTextures/" + item)));
	}

	terrainTextureArray = resourceMan->texManager.loadTextureArrayFromFile("assets/Textures/TerrainTextures/", terrainTextureFileNames);

	terrainVulkanTextureArray = std::make_shared<VulkanTexture2DArray>(renderer->device);
	terrainVulkanTextureArray->loadTextureArray(terrainTextureArray, VK_FORMAT_R8G8B8A8_UNORM, *renderer,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true, 4);

	//WaterTexture = resourceMan->texManager.loadTextureFromFileRGBA("assets/Textures/TileableWaterTexture.jpg");
	//WaterVulkanTexture.loadFromTexture(renderer->device, WaterTexture, VK_FORMAT_R8G8B8A8_UNORM, renderer->device.graphics_queue, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, false, true, 4, true);

	instancedWaters = std::make_unique<InstancedSceneObject>(renderer);
	instancedWaters->SetFragmentShaderToUse(loadShaderModule(renderer->device.device, "assets/shaders/water.frag.spv"));
	instancedWaters->SetBlendMode(VK_TRUE);
	instancedWaters->SetCullMode(VK_CULL_MODE_NONE);
	instancedWaters->LoadModel(createFlatPlane(settings.numCells, glm::vec3(1, 0, 1)));
	instancedWaters->LoadTexture(resourceMan->texManager.loadTextureFromFileRGBA("assets/Textures/TileableWaterTexture.jpg"));

	instancedWaters->InitInstancedSceneObject(renderer);

}

void TerrainManager::CleanUpResources() {
	terrainVulkanTextureArray->destroy();

	instancedWaters->CleanUp();
	//WaterVulkanTexture.destroy(renderer->device);

	//	WaterModel.destroy(renderer->device);
}

void TerrainManager::GenerateTerrain(std::shared_ptr<ResourceManager> resourceMan, std::shared_ptr<VulkanRenderer> renderer, std::shared_ptr<Camera> camera) {
	this->renderer = renderer;
	settings.width = nextTerrainWidth;

	ResetWorkerThreads();

	for (int i = 0; i < settings.gridDimentions; i++) { //creates a grid of terrains centered around 0,0,0
		for (int j = 0; j < settings.gridDimentions; j++) {


			TerrainCoordinateData coord = TerrainCoordinateData(
				glm::vec2((i - settings.gridDimentions / 2) * settings.width - settings.width / 2, (j - settings.gridDimentions / 2) * settings.width - settings.width / 2), //position
				glm::vec2(settings.width, settings.width), //size
				glm::i32vec2(i*settings.sourceImageResolution, j*settings.sourceImageResolution), //noise position
				glm::vec2(1.0 / (float)settings.sourceImageResolution, 1.0f / (float)settings.sourceImageResolution),//noiseSize 
				settings.sourceImageResolution + 1);

			terrainCreationWork.push_back(TerrainCreationData(
				resourceMan, renderer, camera, terrainVulkanTextureArray,
				terrainQuadPool, protoGraph,
				settings.numCells, settings.maxLevels, settings.sourceImageResolution, settings.heightScale,
				coord));

			std::lock_guard<std::mutex> lk(workerMutex);
			workerConditionVariable.notify_one();
		}
	}
	

	//WaterMesh.reset();
	//WaterModel.destroy(renderer->device); 
	//WaterMesh = createFlatPlane(settings.numCells, glm::vec3(settings.width, 0, settings.width));
	//WaterModel.loadFromMesh(WaterMesh, renderer->device, renderer->device.graphics_queue);



	//instancedWaters->CleanUp();
	//instancedWaters.release();
	//instancedWaters = std::make_unique<InstancedSceneObject>(renderer);
	//instancedWaters->SetBlendMode(VK_TRUE);
	//instancedWaters->SetCullMode(VK_CULL_MODE_NONE);
	//instancedWaters->SetFragmentShaderToUse(loadShaderModule(renderer->device.device, "assets/shaders/water.frag.spv"));
	//instancedWaters->LoadModel(createFlatPlane(settings.numCells, glm::vec3(settings.width, 0, settings.width)));
	//instancedWaters->LoadTexture(resourceMan->texManager.loadTextureFromFileRGBA("assets/Textures/TileableWaterTexture.jpg"));
	//instancedWaters->InitInstancedSceneObject(renderer);


	std::vector<InstancedSceneObject::InstanceData> waterData;
	for (int i = 0; i < settings.gridDimentions; i++) {
		for (int j = 0; j < settings.gridDimentions; j++) {
			InstancedSceneObject::InstanceData id;
			id.pos = glm::vec3((i - settings.gridDimentions / 2) * settings.width - settings.width / 2,
				0, (j - settings.gridDimentions / 2) * settings.width - settings.width / 2);
			id.rot = glm::vec3(0, 0, 0);
			id.scale = settings.width;
			waterData.push_back(id);
			//instancedWaters->AddInstance(id);

			//auto water = std::make_shared< Water>
			//	(64, (i - settings.gridDimentions / 2) * settings.width - settings.width / 2, (j - settings.gridDimentions / 2) * settings.width - settings.width / 2, settings.width, settings.width);
			//
			////WaterTexture = resourceMan->texManager.loadTextureFromFileRGBA("assets/Textures/TileableWaterTexture.jpg");
			//water->InitWater(renderer, WaterVulkanTexture);
			//
			//waters.push_back(water);
		}
	}
	instancedWaters->ReplaceAllInstances(waterData);
	//instancedWaters->UploadInstances();
	recreateTerrain = false;
}

void TerrainManager::UpdateTerrains(std::shared_ptr<ResourceManager> resourceMan, std::shared_ptr<VulkanRenderer> renderer, std::shared_ptr<Camera> camera, std::shared_ptr<TimeManager> timeManager) {
	this->renderer = renderer;
	if (recreateTerrain) {
		CleanUpTerrain();
		GenerateTerrain(resourceMan, renderer, camera);
	}

	terrainUpdateTimer.StartTimer();
	terrain_mutex.lock();
	for (auto& ter : terrains) {

		ter->UpdateTerrain(camera->Position);

		//if (terrainUpdateTimer.GetElapsedTimeMicroSeconds() > 1000) {
		//	Log::Debug << terrainUpdateTimer.GetElapsedTimeMicroSeconds() << "\n";
		//}
	}
	terrain_mutex.unlock();
	terrainUpdateTimer.EndTimer();

}

void TerrainManager::RenderTerrain(VkCommandBuffer commandBuffer, bool wireframe) {
	{
		std::lock_guard<std::mutex> lock(terrain_mutex);
		for (auto& ter : terrains) {
			ter->DrawTerrain(commandBuffer, wireframe);
		}
	}

	instancedWaters->WriteToCommandBuffer(commandBuffer, wireframe);
}

//TODO : Reimplement getting height at terrain location
float TerrainManager::GetTerrainHeightAtLocation(float x, float z) {
	for (auto& terrain : terrains)
	{
		glm::vec2 pos = terrain->coordinateData.pos;
		glm::vec2 size = terrain->coordinateData.size;
		if (pos.x <= x && pos.x + size.x >= x && pos.y <= z && pos.y + size.y >= z) {
			return terrain->GetHeightAtLocation((x - pos.x) / settings.width, (z - pos.y) / settings.width);
		}
	}
	return 0;
}

void TerrainManager::SaveSettingsToFile() {
	nlohmann::json j;

	j["show_window"] = settings.show_terrain_manager_window;
	j["terrain_width"] = settings.width;
	j["height_scale"] = settings.heightScale;
	j["max_levels"] = settings.maxLevels;
	j["grid_dimentions"] = settings.gridDimentions;
	j["view_distance"] = settings.viewDistance;
	j["souce_iamge_resolution"] = settings.sourceImageResolution;

	std::ofstream outFile(TerrainSettingsFileName);
	outFile << std::setw(4) << j;
	outFile.close();
}

void TerrainManager::LoadSettingsFromFile() {
	if (fileExists(TerrainSettingsFileName)) {
		std::ifstream input(TerrainSettingsFileName);

		nlohmann::json j;
		input >> j;

		settings.show_terrain_manager_window = j["show_window"];
		settings.width = j["terrain_width"];
		settings.heightScale = j["height_scale"];
		settings.maxLevels = j["max_levels"];
		settings.gridDimentions = j["grid_dimentions"];
		settings.viewDistance = j["view_distance"];
		settings.sourceImageResolution = j["souce_iamge_resolution"];
	}
	else {
		settings = GeneralSettings{};
	}
}


void TerrainManager::UpdateTerrainGUI() {

	ImGui::SetNextWindowSize(ImVec2(200, 220), ImGuiSetCond_FirstUseEver);
	ImGui::SetNextWindowPos(ImVec2(220, 0), ImGuiSetCond_FirstUseEver);
	if (ImGui::Begin("Debug Info", &settings.show_terrain_manager_window)) {

		ImGui::BeginGroup();
		if (ImGui::Button("Save")) {
			SaveSettingsToFile();
		}
		ImGui::SameLine();
		if (ImGui::Button("Load")) {
			LoadSettingsFromFile();
		}
		ImGui::EndGroup();

		ImGui::SliderFloat("Width", &nextTerrainWidth, 100, 10000);
		ImGui::SliderInt("Max Subdivision", &settings.maxLevels, 0, 10);
		ImGui::SliderInt("Grid Width", &settings.gridDimentions, 1, 10);
		ImGui::SliderFloat("Height Scale", &settings.heightScale, 1, 1000);
		ImGui::SliderInt("Image Resolution", &settings.sourceImageResolution, 32, 2048);

		if (ImGui::Button("Recreate Terrain", ImVec2(130, 20))) {
			recreateTerrain = true;
		}

		ImGui::Text("All terrains update Time: %u(uS)", terrainUpdateTimer.GetElapsedTimeMicroSeconds());
		for (auto& ter : terrains)
		{
			ImGui::Text("Terrain Draw Time: %u(uS)", ter->drawTimer.GetElapsedTimeMicroSeconds());
			ImGui::Text("Terrain Quad Count %d", ter->numQuads);
		}
	}
	ImGui::End();

}

void TerrainManager::DrawTerrainTextureViewer() {

	ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiSetCond_FirstUseEver);
	ImGui::SetNextWindowPos(ImVec2(0, 475), ImGuiSetCond_FirstUseEver);


	if (ImGui::Begin("Textures", &drawWindow, ImGuiWindowFlags_MenuBar))
	{

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Open Texture")) {

				}
				if (ImGui::MenuItem("Close"))
					drawWindow = false;
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		// left
		ImGui::BeginChild("left pane", ImVec2(150, 0), true);
		for (int i = 0; i < terrainTextureHandles.size(); i++)
		{
			//char label[128];
			//sprintf(label, "%s", terrainTextureHandles[i].name.c_str());
			if (ImGui::Selectable(terrainTextureHandles[i].name.c_str(), selectedTexture == i))
				selectedTexture = i;
		}

		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginGroup();
		ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing())); // Leave room for 1 line below us
		ImGui::Text("MyObject: %d", selectedTexture);
		ImGui::Separator();


		ImGui::TextWrapped("TODO: Add texture preview");

		ImGui::EndChild();

		ImGui::BeginChild("buttons");
		if (ImGui::Button("PlaceHolder")) {}
		ImGui::EndChild();

		ImGui::EndGroup();

	}
	ImGui::End();

}

void TerrainManager::RecreateTerrain() {
	recreateTerrain = true;
}

void TerrainManager::CleanUpTerrain() {

	terrains.clear();
	waters.clear();
	//delete terrainQuadPool;
	//terrainQuadPool = new MemoryPool<TerrainQuadData, 2 * sizeof(TerrainQuadData)>();
}

