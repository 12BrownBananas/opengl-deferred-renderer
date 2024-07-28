#include <camera.h>
#include <glfw3.h>
#include <state_management.h>
#include <algorithm>

namespace State {
	KeyboardInput::KeyboardInput(unsigned int key_id) {
		key = key_id;
		pressed = false;
	}
	bool KeyboardInput::press(GLFWwindow* window) {
		if (!pressed && glfwGetKey(window, key) == GLFW_PRESS) {
			pressed = true;
			return true;
		}
		return false;
	}
	bool KeyboardInput::isPressed(GLFWwindow* window) {
		press(window);
		return pressed && glfwGetKey(window, key) == GLFW_PRESS;
	}
	bool KeyboardInput::release(GLFWwindow* window) {
		if (pressed && glfwGetKey(window, key) == GLFW_RELEASE) {
			pressed = false;
			return true;
		}
		return false;
	}

	InputManager::InputManager() {
		toggle = KeyboardInput(GLFW_KEY_TAB);
		quit = KeyboardInput(GLFW_KEY_ESCAPE);
		up = KeyboardInput(GLFW_KEY_W);
		down = KeyboardInput(GLFW_KEY_S);
		left = KeyboardInput(GLFW_KEY_A);
		right = KeyboardInput(GLFW_KEY_D);
		opt1 = KeyboardInput(GLFW_KEY_Q);
		opt2 = KeyboardInput(GLFW_KEY_E);
		action = KeyboardInput(GLFW_KEY_SPACE);
	}
	void InputManager::reset(GLFWwindow* window) {
		toggle.release(window);
		quit.release(window);
		up.release(window);
		down.release(window);
		left.release(window);
		right.release(window);
		opt1.release(window);
		opt2.release(window);
		action.release(window);
	}

	void GameState::togglePolygonMode() {
		if (polygonMode == GL_FILL)
			polygonMode = GL_LINE;
		else
			polygonMode = GL_FILL;
		glPolygonMode(GL_FRONT_AND_BACK, polygonMode);
	}
	void GameState::toggleRenderGBuffer() {
		renderGBuffer = !renderGBuffer;
	}

	void GameState::increaseBlendAmount() {
		blendAmount = std::min(blendAmount + 0.1, 1.0);
	}
	void GameState::decreaseBlendAmount() {
		blendAmount = std::max(blendAmount - 0.1, 0.0);
	}
	void GameState::moveCameraFore() {
		camera.processKeyboard(CAMERA_H::FORWARD, deltaTime, flyEnabled);
	}
	void GameState::moveCameraAft() {
		camera.processKeyboard(CAMERA_H::BACKWARD, deltaTime, flyEnabled);
	}
	void GameState::moveCameraLeft() {
		camera.processKeyboard(CAMERA_H::LEFT, deltaTime, flyEnabled);
	}
	void GameState::moveCameraRight() {
		camera.processKeyboard(CAMERA_H::RIGHT, deltaTime, flyEnabled);
	}
	void GameState::moveCameraUp() {
		camera.processKeyboard(CAMERA_H::UP, deltaTime, flyEnabled);
	}
	void GameState::moveCameraDown() {
		camera.processKeyboard(CAMERA_H::DOWN, deltaTime, flyEnabled);
	}
	void GameState::toggleShadows() {
		shadows = !shadows;
	}
}