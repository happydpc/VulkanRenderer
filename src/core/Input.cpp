#include "Input.h"

#include <algorithm>
#include <GLFW/glfw3.h>

#include <glm/fwd.hpp>

#include "Logger.h"


//External facing access to buttons. 
namespace Input {
	template <typename Enumeration>
	auto as_integer(Enumeration const value) -> typename std::underlying_type<Enumeration>::type
	{
		return static_cast<typename std::underlying_type<Enumeration>::type>(value);
	}

	InputDirector inputDirector;

	void SetupJoysticks() {
		inputDirector.SetupJoysticks();
	}

	bool GetKey(KeyCode code) {
		return inputDirector.GetKey(code);
	}

	bool GetKeyDown(KeyCode code) {
		return inputDirector.GetKeyDown(code);
	}

	bool GetKeyUp(KeyCode code) {
		return inputDirector.GetKeyUp(code);
	}

	bool GetMouseButton(int button) {
		return inputDirector.GetMouseButton(button);
	}
	bool GetMouseButtonPressed(int button) {
		return inputDirector.GetMouseButtonPressed(button);
	}
	bool GetMouseButtonReleased(int button) {
		return inputDirector.GetMouseButtonReleased(button);
	}
	glm::vec2 GetMousePosition() {
		return inputDirector.GetMousePosition();
	}
	glm::vec2 GetMouseChangeInPosition() {
		return inputDirector.GetMouseChangeInPosition();
	}
	float GetMouseScrollX() {
		return inputDirector.GetMouseScrollX();
	}
	float GetMouseScrollY() {
		return inputDirector.GetMouseScrollY();
	}

	void SetTextInputMode() {
		inputDirector.SetTextInputMode();
	}
	void ResetTextInputMode() {
		inputDirector.ResetTextInputMode();
	}
	bool GetTextInputMode() {
		return inputDirector.GetTextInputMode();
	}

	void ConnectJoystick(int index) {
		inputDirector.ConnectJoystick(index);
	}
	void DisconnectJoystick(int index) {
		inputDirector.DisconnectJoystick(index);
	}

	bool IsJoystickConnected(int index) {
		return inputDirector.IsJoystickConnected(index);
	}

	float GetControllerAxis(int controllerID, int axis) {
		return inputDirector.GetControllerAxis(controllerID, axis);
	}
	bool GetControllerButton(int controllerID, int button) {
		return inputDirector.GetControllerButton(controllerID, button);
	}


	InputDirector::InputDirector() :
		mousePosition(glm::vec2(0, 0)), mousePositionPrevious(glm::vec2(0, 0)),
		mouseChangeInPosition(glm::vec2(0, 0)), mouseScroll(glm::vec2(0, 0))
	{
	}

	void InputDirector::SetupJoysticks() {
		for (int i = 0; i < 16; i++) {
			joystickData[i].joystickIndex = i;
			if (glfwJoystickPresent(i)) {
				joystickData[i].connected = true;
				joystickData[i].axes = glfwGetJoystickAxes(i, &(joystickData[i].axesCount));
				joystickData[i].buttons = glfwGetJoystickButtons(i, &(joystickData[i].buttonCount));

			}
		}
	}

	void InputDirector::JoystickData::Connect() {

		if (glfwJoystickPresent(joystickIndex)) {
			connected = true;
			axes = glfwGetJoystickAxes(joystickIndex, &axesCount);
			buttons = glfwGetJoystickButtons(joystickIndex, &buttonCount);
		}
		else 
			Disconnect();
	}

	void InputDirector::JoystickData::Disconnect() {
		connected = false;
		axes = nullptr;
		buttons = nullptr;

	}

	bool InputDirector::JoystickData::IsConnected() {
		return connected;
	}

	std::vector<int> InputDirector::GetConnectedJoysticks() {
		std::vector<int> joys;

		for (auto possible : joystickData)
			if (possible.IsConnected())
				joys.push_back(possible.joystickIndex);

		return joys;
	}

	float InputDirector::JoystickData::GetAxis(int index) {
		if (index >= 0 && index < axesCount)
			if (axes != nullptr)
				return axes[index];
		else
			Log::Error << "Tried to access axes that is out of bounds! \n";
	}

	bool InputDirector::JoystickData::GetButton(int index) {
		if (index >= 0 && index < buttonCount)
			if(buttons != nullptr)
				return buttons[index];
		else
			Log::Error << "Tried to access button that is out of bounds! \n";
	}

	bool InputDirector::GetKey(KeyCode code) {
		return keys[as_integer(code)];
	}

	bool InputDirector::GetKeyDown(KeyCode code) {
		return keysDown[as_integer(code)];
	}

	bool InputDirector::GetKeyUp(KeyCode code) {
		return keysUp[as_integer(code)];
	}

	bool InputDirector::GetMouseButton(int button) {
		return mouseButtons[button];
	}
	bool InputDirector::GetMouseButtonPressed(int button) {
		return mouseButtonsDown[button];
	}
	bool InputDirector::GetMouseButtonReleased(int button) {
		return mouseButtonsUp[button];
	}
	glm::vec2 InputDirector::GetMousePosition() {
		return mousePosition;
	}
	glm::vec2 InputDirector::GetMouseChangeInPosition() {
		return mouseChangeInPosition;
	}
	float InputDirector::GetMouseScrollX() {
		return mouseScroll.x;
	}
	float InputDirector::GetMouseScrollY() {
		return mouseScroll.y;
	}
	void InputDirector::SetTextInputMode() {
		textInputMode = true;
	}
	void InputDirector::ResetTextInputMode() {
		textInputMode = false;
	}
	bool InputDirector::GetTextInputMode() {
		return textInputMode;
	}

	void InputDirector::ConnectJoystick(int index) {
		if (index >= 0 && index < 16) 
			joystickData[index].Connect();	
		else
			Log::Error << "Tried to connect joystick that is out of bounds!\n";
	}

	void InputDirector::DisconnectJoystick(int index) {
		if (index >= 0 && index < 16) 
			joystickData[index].Disconnect();
		else
			Log::Error << "Tried to disconnect joystick that is out of bounds!\n";
	}

	bool InputDirector::IsJoystickConnected(int index) {
		if (index >= 0 && index < 16)
			return joystickData[index].IsConnected();
		else
			Log::Error << "Tried to test if an out of range joystick is connected!\n";
	}

	float InputDirector::GetControllerAxis(int id, int axis) {
		if (id >= 0 && id < 16) 
			return joystickData[id].GetAxis(axis);
		else
			Log::Error << "Tried to access joystick axis that is out of bounds!\n";
	}

	bool InputDirector::GetControllerButton(int id, int button) {
		if (id >= 0 && id < 16)
			return joystickData[id].GetButton(button);
		else
			Log::Error << "Tried to access joystick button that is out of bounds!\n";
	}


	void InputDirector::UpdateInputs() {
		glfwPollEvents();
		for (int i = 0; i < 16; i++) {
			if (glfwJoystickPresent(i)) {
				joystickData[i].connected = true;
				joystickData[i].axes = glfwGetJoystickAxes(i, &(joystickData[i].axesCount));
				joystickData[i].buttons = glfwGetJoystickButtons(i, &(joystickData[i].buttonCount));

			}
			else {
				joystickData[i].connected = false;
				joystickData[i].axes = nullptr;
				joystickData[i].buttons = nullptr;
			}
		}
	}

	void InputDirector::ResetReleasedInput() {
		std::fill(std::begin(keysDown), std::end(keysDown), 0);
		std::fill(std::begin(keysUp), std::end(keysUp), 0);
		std::fill(std::begin(mouseButtonsUp), std::end(mouseButtonsUp), 0);
		std::fill(std::begin(mouseButtonsDown), std::end(mouseButtonsDown), 0);
		mouseScroll = glm::vec2(0, 0);
		mouseChangeInPosition = glm::dvec2(0, 0);
	}

	void InputDirector::keyEvent(int key, int scancode, int action, int mods) {
		switch (action) {
		case GLFW_PRESS:
			keys[key] = true;
			keysDown[key] = true;
			break;

		case GLFW_REPEAT:
			keysDown[key] = false;
			break;

		case GLFW_RELEASE:
			keys[key] = false;
			keysUp[key] = true;
			break;

		default:
			break;
		}
	}

	void InputDirector::mouseButtonEvent(int button, int action, int mods) {
		switch (action) {
		case GLFW_PRESS:
			mouseButtons[button] = true;
			mouseButtonsDown[button] = true;
			break;

		case GLFW_REPEAT:
			mouseButtonsDown[button] = false;
			break;

		case GLFW_RELEASE:
			mouseButtons[button] = false;
			mouseButtonsUp[button] = true;
			break;

		default:
			break;
		}
	}

	void InputDirector::mouseMoveEvent(double xoffset, double yoffset) {
		mousePosition = glm::dvec2(xoffset, yoffset);

		mouseChangeInPosition = mousePositionPrevious - mousePosition;
		mouseChangeInPosition.x *= -1; //coordinates are reversed on y axis (top left vs bottom left)

		mousePositionPrevious = mousePosition;

		//glfwGetCursorPos(glfwGetCurrentContext(), &mousePosition.x, &mousePosition.y);
	}

	void InputDirector::mouseScrollEvent(double xoffset, double yoffset) {
		mouseScroll = glm::dvec2(xoffset, yoffset);
	}
}