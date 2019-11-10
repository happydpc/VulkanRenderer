#pragma once


#include <memory>

#include "imgui.hpp"

#include "core/Engine.h"

#include "ProcTerrainNodeGraph.h"


struct ImGUI_PanelSettings
{
	bool showGui = true;
	bool camera_controls = true;
	bool log = true;
	bool debug_overlay = true;
	bool controls_list = true;
	bool controller_list = true;
};

class Editor
{
	public:
	Editor (Engine& engine);

	void UpdateInputs ();

	void DrawImgui ();

	Engine& engine;
	void DebugOverlay (bool* show_debug_overlay);
	void CameraWindow (bool* show_camera_overlay);
	void ControlsWindow (bool* show_controls_window);
	void ControllerWindow (bool* show_controller_window);

	ImGUI_PanelSettings panels;

	ProcTerrainNodeGraph imgui_nodeGraph_terrain;
	bool debug_mode = true;
};