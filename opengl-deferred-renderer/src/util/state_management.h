#ifndef STATE_MANAGEMENT_H
#define STATE_MANAGEMENT_H

#include <camera.h>
#include <glfw3.h>
namespace State {
	class KeyboardInput {
		public:
			unsigned int key;
			bool pressed;

			KeyboardInput(unsigned int key_id);
			bool press(GLFWwindow* window);
			bool isPressed(GLFWwindow* window);
			bool release(GLFWwindow* window);
	};

	class InputManager {
		public:
			KeyboardInput toggle = NULL;
			KeyboardInput quit = NULL;
			KeyboardInput up = NULL;
			KeyboardInput down = NULL;
			KeyboardInput left = NULL;
			KeyboardInput right = NULL;
			KeyboardInput opt1 = NULL;
			KeyboardInput opt2 = NULL;
			KeyboardInput action = NULL;

			InputManager();
			void reset(GLFWwindow* window);
	};

	class GameState {
		public:
			GameState() = default;
			float deltaTime = 0.0f;
			float lastFrame = 0.0f;

			float lastX = 400, lastY = 300;
			bool firstMouse = true;

			bool flyEnabled = true;
			bool shadows = true;

			float blendAmount = 0.2;
			InputManager inputManager;
			Camera camera;
			bool renderGBuffer = false;
			void togglePolygonMode();
			void toggleRenderGBuffer();
			void increaseBlendAmount();
			void decreaseBlendAmount();
			void moveCameraFore();
			void moveCameraAft();
			void moveCameraLeft();
			void moveCameraRight();
			void moveCameraUp();
			void moveCameraDown();
			void toggleShadows();
		protected:
			unsigned int polygonMode = GL_FILL;

	};
}
#endif