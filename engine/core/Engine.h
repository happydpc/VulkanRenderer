#pragma once

#include <filesystem>

#include "Input.h"
#include "JobSystem.h"
#include "Logger.h"
#include "Time.h"
#include "Window.h"
#include "util/SimpleTimer.h"

#include "resources/Resource.h"

#include "rendering/Renderer.h"

#include "scene/Scene.h"

class EngineSettings
{
	public:
	EngineSettings (std::filesystem::path fileName);
	void Load ();
	void Save ();

	int screenWidth = 800;
	int screenHeight = 600;
	bool isFullscreen = false;

	bool useValidationLayers = true;

	bool isFrameCapped = true;
	double MaxFPS = 100.0f;

	private:
	std::filesystem::path fileName;
};

struct StaticInitializer
{
	StaticInitializer ();
	~StaticInitializer ();
};

class Engine
{
	public:
	Engine ();
	~Engine ();
	Engine (Engine const& app) = delete;
	Engine& operator= (Engine const& app) = delete;
	Engine (Engine&& app) = delete;
	Engine& operator= (Engine&& app) = delete;

	void SetImguiUpdateCallBack (std::function<void ()> cb) { imgui_update_callback = cb; }
	void SetImguiDrawCallBack (std::function<void ()> cb) { imgui_draw_callback = cb; }

	void Run ();
	void HandleInputs ();

	EngineSettings settings;

	StaticInitializer static_initializer;

	job::TaskManager task_manager;

	Time time_manager;
	Window window;
	Input::InputDirector input;
	Resource::ResourceManager resource_manager;

	VulkanRenderer vulkan_renderer;
	Scene scene;

	private:
	std::function<void ()> imgui_update_callback;
	std::function<void ()> imgui_draw_callback;

	void BuildImgui ();

	// ImGui resources
	SimpleTimer imGuiTimer;
};
