#pragma once

#include "rendering/Renderer.h"

namespace job
{
class TaskManager;
}
class Time;
namespace Resource
{
class ResourceManager;
}
class VulkanRenderer;

class Scene
{
	public:
	Scene (job::TaskManager& task_manager, Time& time_manager, Resource::ResourceManager& resourceMan, VulkanRenderer& renderer);

	void Update ();

	private:
	job::TaskManager& task_manager;
	Time& time_manager;
	Resource::ResourceManager& resource_manager;
	VulkanRenderer& renderer;
};


//#include "Camera.h"
//#include "Skybox.h"
//#include "Terrain.h"
//#include "TerrainManager.h"
//#include "Water.h"
//
// struct SkySettings
//{
//	bool show_skyEditor = true;
//	bool autoMove = false;
//	float moveSpeed = 0.0002f;
//	float horizontalAngle = 0.0f;
//	float verticalAngle = 1.0f;
//
//	DirectionalLight sun;
//	DirectionalLight moon;
//};
//
// class Scene
//{
//	public:
//	Scene (job::TaskManager& task_manager,
//	    Resource::ResourceManager& resourceMan,
//	    VulkanRenderer& renderer,
//	    Time& time_manager,
//	    InternalGraph::GraphPrototype& graph);
//
//	void UpdateScene ();
//	void RenderDepthPrePass (VkCommandBuffer commandBuffer);
//	void RenderOpaque (VkCommandBuffer commandBuffer, bool wireframe);
//	void RenderTransparent (VkCommandBuffer commandBuffer, bool wireframe);
//	void RenderSkybox (VkCommandBuffer commandBuffer);
//	void UpdateSceneGUI ();
//
//	Camera* GetCamera ();
//
//	bool drawNormals = false;
//	bool walkOnGround = false;
//
//	private:
//	job::TaskManager& task_manager;
//	VulkanRenderer& renderer;
//	Resource::ResourceManager& resourceMan;
//	Time& time_manager;
//
//	std::vector<DirectionalLight> directionalLights;
//	std::vector<PointLight> pointLights;
//	std::vector<SpotLight> spotLights;
//
//	std::unique_ptr<TerrainManager> terrainManager;
//	std::unique_ptr<Water> water_plane;
//
//	std::unique_ptr<Skybox> skybox;
//
//	std::unique_ptr<Camera> camera;
//
//	float verticalVelocity = 0;
//	float gravity = -0.25f;
//	float heightOfGround = 1.4f;
//
//	SkySettings skySettings;
//	void UpdateSunData ();
//	void DrawSkySettingsGui ();
//
//	bool pressedControllerJumpButton = false;
//	bool releasedControllerJumpButton = false;
//
//	bool UpdateTerrain = true;
//};
