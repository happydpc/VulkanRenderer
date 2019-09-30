#pragma once

#include "cml/cml.h"

#include "rendering/Renderer.h"

#include "resources/Resource.h"

#include "core/TimeManager.h"

#include "Camera.h"
#include "GameObject.h"
#include "InstancedSceneObject.h"
#include "Skybox.h"
#include "Terrain.h"
#include "TerrainManager.h"
#include "Water.h"

#include "Transform.h"

//#include <gltf2\glTF2.hpp>

struct SkySettings
{
	bool show_skyEditor = true;
	bool autoMove = false;
	float moveSpeed = 0.0002f;
	float horizontalAngle = 0.0f;
	float verticalAngle = 1.0f;

	DirectionalLight sun;
	DirectionalLight moon;
};

class Scene
{
	public:
	Scene (Resource::AssetManager& resourceMan,
	    VulkanRenderer& renderer,
	    TimeManager& timeManager,
	    InternalGraph::GraphPrototype& graph);
	~Scene ();

	void UpdateScene ();
	void RenderDepthPrePass (VkCommandBuffer commandBuffer);
	void RenderOpaque (VkCommandBuffer commandBuffer, bool wireframe);
	void RenderTransparent (VkCommandBuffer commandBuffer, bool wireframe);
	void RenderSkybox (VkCommandBuffer commandBuffer);
	void UpdateSceneGUI ();

	Camera* GetCamera ();

	bool drawNormals = false;
	bool walkOnGround = false;

	private:
	VulkanRenderer& renderer;
	Resource::AssetManager& resourceMan;
	TimeManager& timeManager;

	std::vector<DirectionalLight> directionalLights;
	std::vector<PointLight> pointLights;
	std::vector<SpotLight> spotLights;

	std::vector<std::unique_ptr<GameObject>> gameObjects;
	std::unique_ptr<TerrainManager> terrainManager;
	std::unique_ptr<Water> water_plane;
	std::unique_ptr<InstancedSceneObject> treesInstanced;
	std::unique_ptr<InstancedSceneObject> rocksInstanced;

	std::unique_ptr<Skybox> skybox;

	std::unique_ptr<Camera> camera;

	float verticalVelocity = 0;
	float gravity = -0.25f;
	float heightOfGround = 1.4f;

	SkySettings skySettings;
	void UpdateSunData ();
	void DrawSkySettingsGui ();

	bool pressedControllerJumpButton = false;
	bool releasedControllerJumpButton = false;

	bool UpdateTerrain = true;

	PBR_Material testMat;
	InstancedSceneObject::InstanceData testInstanceData;
};
