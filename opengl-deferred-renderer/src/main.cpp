#include <iostream>
#include <string>
#include <stb_image.h>
#include <vector>
#include <map>
#include <cmath>
#include <random>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glfw3.h>

#include <shader_util.h>
#include <state_management.h>
#include <camera.h>
#include <render_util.h>

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

const float LIGHT_ATTENUATION_CONSTANT = 1.0f;
const float LIGHT_ATTENUATION_LINEAR = 0.09f;
const float LIGHT_ATTENUATION_QUADRATIC = 0.032f;
const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024; //NOTE: Used for shadow map generation, which is not currently in this demo

State::GameState state;

void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window, State::GameState& state);

unsigned int generate_noise_texture(std::vector<glm::vec3> ssaoNoise);
void render_cube();
void render_quad();
void draw_scaled(Model& scaledModel, Shader& shader, glm::vec3 scale);
void draw_outline(Model& outlineModel, Shader& outlineShader, glm::vec3 scale);
std::map<float, glm::vec3> get_sorted_position_map(std::vector<glm::vec3> positions);

static float naive_lerp(float a, float b, float t)
{
	return a + t * (b - a);
}

int main() {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	stbi_set_flip_vertically_on_load(false);
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL Deferred Renderer", NULL, NULL);
	if (window == NULL) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glEnable(GL_DEPTH_TEST);
	Shader lightingShader = Shader("./src/shaders/lighting/shd_deferred_lighting.vert", "./src/shaders/lighting/shd_deferred_lighting.frag");
	Shader forwardLightingShader = Shader("./src/shaders/lighting/shd_forward_lighting.vert", "./src/shaders/lighting/shd_forward_lighting.frag");

	Shader blurShader = Shader("./src/shaders/postprocessing/shd_gaussian_blur.vert", "./src/shaders/postprocessing/shd_gaussian_blur.frag");
	Shader hdrShader = Shader("./src/shaders/postprocessing/shd_hdr.vert", "./src/shaders/postprocessing/shd_hdr.frag");
	Shader screenShader = Shader("./src/shaders/postprocessing/shd_2d.vert", "./src/shaders/postprocessing/shd_2d.frag");
	Shader simpleBlurShader = Shader("./src/shaders/postprocessing/shd_simple_blur.vert", "./src/shaders/postprocessing/shd_simple_blur.frag");

	Shader gBufferShader = Shader("./src/shaders/shd_gbuffer.vert", "./src/shaders/shd_gbuffer.frag");
	Shader ssaoShader = Shader("./src/shaders/shd_ssao.vert", "./src/shaders/shd_ssao.frag");

	Model ourModel("./rsc/models/shadow/shadow.obj");
	unsigned int woodTexture = texture_from_file("wood.png", "./rsc/textures/", true);

	/* Generate noise texture (for SSAO) */
	std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
	std::default_random_engine generator;
	std::vector<glm::vec3> ssaoKernel;
	for (size_t i = 0; i < 64; ++i) {
		glm::vec3 sample(
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator)
		);
		sample = glm::normalize(sample);
		sample *= randomFloats(generator);
		float scale = (float)i / 64.0;
		scale = naive_lerp(0.1f, 1.0f, scale * scale);
		sample *= scale;
		ssaoKernel.push_back(sample);
		ssaoKernel.push_back(sample);
	}

	std::vector<glm::vec3> ssaoNoise;
	for (size_t i = 0; i < 16; i++) {
		glm::vec3 noise(
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator) * 2.0 - 1.0,
			0.0f);
		ssaoNoise.push_back(noise);
	}
	unsigned int noiseTexture = generate_noise_texture(ssaoNoise);
	

	/* Create the "screen" buffer */
	Framebuffer::Framebuffer forwardBuffer(SCR_WIDTH, SCR_HEIGHT);
	forwardBuffer.init(true, GL_DEPTH_COMPONENT, GL_DEPTH_ATTACHMENT);
	forwardBuffer.generateTexture(GL_COLOR_ATTACHMENT0, GL_RGBA16F, GL_RGBA);
	if (!Framebuffer::check_if_bound_framebuffer_is_complete()) {
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
		std::cout << glCheckFramebufferStatus(GL_FRAMEBUFFER) << std::endl;
	}
	Framebuffer::reset_framebuffer_bindings();

	/* Next, the deferred rendering buffer */
	Framebuffer::Framebuffer gBuffer(SCR_WIDTH, SCR_HEIGHT);
	gBuffer.init(true, GL_DEPTH_COMPONENT, GL_DEPTH_ATTACHMENT);
	gBuffer.generateTexture(GL_COLOR_ATTACHMENT0, GL_RGBA16F, GL_RGBA, NULL, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
	gBuffer.generateTexture(GL_COLOR_ATTACHMENT1, GL_RGBA16F, GL_RGBA, NULL, GL_NEAREST, GL_NEAREST);
	gBuffer.generateTexture(GL_COLOR_ATTACHMENT2, GL_RGBA, GL_RGBA, NULL, GL_NEAREST, GL_NEAREST);
	gBuffer.setDrawBuffers();
	if (!Framebuffer::check_if_bound_framebuffer_is_complete()) {
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
		std::cout << glCheckFramebufferStatus(GL_FRAMEBUFFER) << std::endl;
	}
	Framebuffer::reset_framebuffer_bindings();

	/* Next, the "result" buffer of the intermediate lighting step */
	Framebuffer::Framebuffer deferredBuffer(SCR_WIDTH, SCR_HEIGHT);
	deferredBuffer.init(true, GL_DEPTH_COMPONENT, GL_DEPTH_ATTACHMENT);
	deferredBuffer.generateTexture(GL_COLOR_ATTACHMENT0, GL_RGBA16F, GL_RGBA, NULL, GL_NEAREST, GL_NEAREST);
	if (!Framebuffer::check_if_bound_framebuffer_is_complete()) {
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
		std::cout << glCheckFramebufferStatus(GL_FRAMEBUFFER) << std::endl;
	}
	Framebuffer::reset_framebuffer_bindings();

	/* The framebuffer to write ssao data to */
	Framebuffer::Framebuffer ssaoFramebuffer(SCR_WIDTH, SCR_HEIGHT);
	ssaoFramebuffer.init(false);
	ssaoFramebuffer.generateTexture(GL_COLOR_ATTACHMENT0, GL_RED, GL_RED);
	if (!Framebuffer::check_if_bound_framebuffer_is_complete()) {
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
		std::cout << glCheckFramebufferStatus(GL_FRAMEBUFFER) << std::endl;
	}
	Framebuffer::reset_framebuffer_bindings();
	/* And the framebuffer to hold the blurred result of the ssao pass */
	Framebuffer::Framebuffer ssaoBlurFramebuffer(SCR_WIDTH, SCR_HEIGHT);
	ssaoBlurFramebuffer.init(false);
	ssaoBlurFramebuffer.generateTexture(GL_COLOR_ATTACHMENT0, GL_RED, GL_RED);
	if (!Framebuffer::check_if_bound_framebuffer_is_complete()) {
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
		std::cout << glCheckFramebufferStatus(GL_FRAMEBUFFER) << std::endl;
	}
	Framebuffer::reset_framebuffer_bindings();

	std::vector<glm::vec3> objectPositions;
	objectPositions.push_back(glm::vec3(-3.0, -0.5, -3.0));
	objectPositions.push_back(glm::vec3(0.0, -0.5, -3.0));
	objectPositions.push_back(glm::vec3(3.0, -0.5, -3.0));
	objectPositions.push_back(glm::vec3(-3.0, -0.5, 0.0));
	objectPositions.push_back(glm::vec3(0.0, -0.5, 0.0));
	objectPositions.push_back(glm::vec3(3.0, -0.5, 0.0));
	objectPositions.push_back(glm::vec3(-3.0, -0.5, 3.0));
	objectPositions.push_back(glm::vec3(0.0, -0.5, 3.0));
	objectPositions.push_back(glm::vec3(3.0, -0.5, 3.0));

	std::vector<float> objectRotations;
	for (size_t i = 0; i < objectPositions.size(); ++i)
		objectRotations.push_back(0.0f);
	const float rotationRate = 160.0f; //degrees per standard unit of time (modified by delta time)

	std::vector<float> objectOscillationOffsets;
	for (int i = 0; i < objectPositions.size(); i++) {
		objectOscillationOffsets.push_back((float)i);
	}
	const float maxDeviance = 1.0f;
	
	// lighting info
	// -------------
	const glm::vec3 ambientLightColor = glm::vec3(0.1);
	const unsigned int NR_LIGHTS = 32;
	std::vector<glm::vec3> lightPositions;
	std::vector<glm::vec3> lightColors;
	srand(13);
	for (unsigned int i = 0; i < NR_LIGHTS; i++)
	{
		// calculate slightly random offsets
		float xPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 3.0);
		float yPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 4.0);
		float zPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 3.0);
		lightPositions.push_back(glm::vec3(xPos, yPos, zPos));
		// also calculate random color
		float rColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5); // between 0.5 and 1.0
		float gColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5); // between 0.5 and 1.0
		float bColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5); // between 0.5 and 1.0
		lightColors.push_back(glm::vec3(rColor, gColor, bColor));
	}

	gBufferShader.use();
	gBufferShader.setInt("texture_diffuse1", 0);
	gBufferShader.setInt("texture_specular1", 1);
	screenShader.use();
	screenShader.setInt("screenTexture", 0);
	lightingShader.use();
	lightingShader.setInt("gPosition", 0);
	lightingShader.setInt("gNormal", 1);
	lightingShader.setInt("gAlbedoSpec", 2);
	lightingShader.setInt("ssao", 3);
	lightingShader.setVec3f("ambientLightColor", ambientLightColor);
	ssaoShader.use();
	ssaoShader.setInt("gPosition", 0);
	ssaoShader.setInt("gNormal", 1);
	ssaoShader.setInt("texNoise", 2);
	ssaoShader.setVec2f("noiseScale", glm::vec2(SCR_WIDTH / 4.0f, SCR_HEIGHT / 4.0f));
	ssaoShader.setInt("kernelSize", 64);
	ssaoShader.setFloat("radius", 0.5f);
	ssaoShader.setFloat("bias", 0.025f);
	simpleBlurShader.use();
	simpleBlurShader.setInt("ssaoInput", 0);

	state.camera.position = glm::vec3(-6.5, 0.0, 0.0);

	while (!glfwWindowShouldClose(window)) {
		//Calculate delta time
		float currentFrame = static_cast<float>(glfwGetTime());
		state.deltaTime = currentFrame - state.lastFrame;
		state.lastFrame = currentFrame;
		
		//Input
		processInput(window, state);

		/* Core Render Loop Begins */
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);

		glm::mat4 projection = glm::perspective(glm::radians(state.camera.zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = state.camera.getViewMatrix();

		gBuffer.bind();
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			gBufferShader.use();
			gBufferShader.setMat4("projection", projection);
			gBufferShader.setMat4("view", view);
			glm::mat4 model;
			for (size_t i = 0; i < objectPositions.size(); ++i) {
				model = glm::mat4(1.0f);
				model = glm::translate(model, objectPositions[i]);
				model = glm::rotate(model, glm::radians(-90.0f+objectRotations[i]), glm::normalize(glm::vec3(0.0, 1.0, 0.0)));
				model = glm::scale(model, glm::vec3(0.025f));
				//now update the model's position and rotation information
				objectPositions[i].y = maxDeviance * glm::sin(glfwGetTime() + objectOscillationOffsets[i]);
				objectRotations[i] = std::fmod(objectRotations[i] + (rotationRate * state.deltaTime), 360.0f);
				gBufferShader.setMat4("model", model);
				gBufferShader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
				ourModel.draw(gBufferShader);
			}
			model = glm::mat4(1.0f);
			model = glm::scale(model, glm::vec3(10.0));
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, woodTexture);
			gBufferShader.setMat4("model", model);
			gBufferShader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
			render_cube();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		ssaoFramebuffer.bind();
			glClear(GL_COLOR_BUFFER_BIT);
			ssaoShader.use();
			glActiveTexture(GL_TEXTURE0);
			gBuffer.bindTexture(GL_COLOR_ATTACHMENT0); //position
			glActiveTexture(GL_TEXTURE1);
			gBuffer.bindTexture(GL_COLOR_ATTACHMENT1); //normal
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, noiseTexture);
			
			for (size_t i = 0; i < ssaoKernel.size(); ++i) {
				ssaoShader.setVec3f("samples["+std::to_string(i)+"]", ssaoKernel[i]);
			}
			ssaoShader.setMat4("projection", projection);
			ssaoShader.setMat4("view", view);
			render_quad();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		ssaoBlurFramebuffer.bind();
			glClear(GL_COLOR_BUFFER_BIT);
			simpleBlurShader.use();
			glActiveTexture(GL_TEXTURE0);
			ssaoFramebuffer.bindTexture(GL_COLOR_ATTACHMENT0);
			render_quad();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		deferredBuffer.bind();
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			lightingShader.use();
			glActiveTexture(GL_TEXTURE0);
			gBuffer.bindTexture(GL_COLOR_ATTACHMENT0); //position
			glActiveTexture(GL_TEXTURE1);
			gBuffer.bindTexture(GL_COLOR_ATTACHMENT1); //normal
			glActiveTexture(GL_TEXTURE2);
			gBuffer.bindTexture(GL_COLOR_ATTACHMENT2); //albedo/specular
			glActiveTexture(GL_TEXTURE3);
			ssaoBlurFramebuffer.bindTexture(GL_COLOR_ATTACHMENT0); //blur
			//send the right uniforms to the lighting shader
			lightingShader.setVec2f("position", glm::vec2(0.0, 0.0));
			lightingShader.setVec2f("scale", glm::vec2(1.0, 1.0));
			for (unsigned int i = 0; i < lightPositions.size(); i++)
			{
				glm::vec3 lightColor = lightColors[i];
				lightingShader.setVec3f("lights[" + std::to_string(i) + "].Position", lightPositions[i]);
				lightingShader.setVec3f("lights[" + std::to_string(i) + "].Color", lightColor);
				// update attenuation parameters and calculate radius
				const float linear = 0.7f;
				const float quadratic = 1.8f;
				const float constant = 1.0;
				lightingShader.setFloat("lights[" + std::to_string(i) + "].Linear", linear);
				lightingShader.setFloat("lights[" + std::to_string(i) + "].Quadratic", quadratic);
				float lightMax = std::fmaxf(std::fmaxf(lightColor.r, lightColor.g), lightColor.b);
				float radius =
					(-linear + std::sqrtf(linear * linear - 4 * quadratic * (constant - (256.0 / 5.0) * lightMax)))
					/ (2 * quadratic);
				lightingShader.setFloat("lights[" + std::to_string(i) + "].Radius", radius);
			}
			lightingShader.setVec3f("viewPos", state.camera.position);
			lightingShader.setBool("useSSAO", !state.renderGBuffer); //simplifies toggling SSAO on and off by reusing the existing state variable
			render_quad();

		gBuffer.bind(GL_READ_FRAMEBUFFER);
		forwardBuffer.bind(GL_DRAW_FRAMEBUFFER);
		//we now blit the depth information into the forward-lighting buffer
		glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

		//and the color information from the deferred buffer
		deferredBuffer.bind(GL_READ_FRAMEBUFFER);
		glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		forwardBuffer.bind();
			forwardLightingShader.use();
			forwardLightingShader.setMat4("projection", projection);
			forwardLightingShader.setMat4("view", view);
			for (size_t i = 0; i < lightPositions.size(); i++) {
				glm::mat4 model = glm::mat4(1.0f);
				model = glm::translate(model, lightPositions[i]);
				model = glm::scale(model, glm::vec3(0.125f));
				forwardLightingShader.setMat4("model", model);
				forwardLightingShader.setVec3f("lightColor", lightColors[i]);
				render_cube();
			}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		screenShader.use();
		screenShader.setBool("grayscale", false);
		if (state.renderGBuffer) {
			glActiveTexture(GL_TEXTURE0);
			gBuffer.bindTexture(GL_COLOR_ATTACHMENT0);
			screenShader.setVec2f("position", glm::vec2(-0.5, 0.5));
			screenShader.setVec2f("scale", glm::vec2(0.5, 0.5));
			render_quad();

			gBuffer.bindTexture(GL_COLOR_ATTACHMENT1);
			screenShader.setVec2f("position", glm::vec2(0.5, 0.5));
			screenShader.setVec2f("scale", glm::vec2(0.5, 0.5));
			render_quad();

			gBuffer.bindTexture(GL_COLOR_ATTACHMENT2);
			screenShader.setVec2f("position", glm::vec2(-0.5, -0.5));
			screenShader.setVec2f("scale", glm::vec2(0.5, 0.5));
			render_quad();
		}
		glActiveTexture(GL_TEXTURE0);
		forwardBuffer.bindTexture(GL_COLOR_ATTACHMENT0); //now we render the combined texture to the screen
		screenShader.setVec2f("position", glm::vec2(0.0, 0.0));
		screenShader.setVec2f("scale", glm::vec2(1.0, 1.0));
		render_quad();

		glBindVertexArray(0);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glfwTerminate();
	return 0;
}

unsigned int generate_noise_texture(std::vector<glm::vec3> ssaoNoise) {
	unsigned int noiseTexture;
	glGenTextures(1, &noiseTexture);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	return noiseTexture;
}

// renderCube() renders a 1x1 3D cube in NDC.
// -------------------------------------------------
unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void render_cube()
{
	// initialize (if necessary)
	if (cubeVAO == 0)
	{
		float vertices[] = {
			// back face
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
			// front face
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			// left face
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			// right face
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
			 // bottom face
			 -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			  1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
			  1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			  1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			 -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			 -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			 // top face
			 -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			  1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			  1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
			  1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			 -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			 -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
		};
		glGenVertexArrays(1, &cubeVAO);
		glGenBuffers(1, &cubeVBO);
		// fill buffer
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		// link vertex attributes
		glBindVertexArray(cubeVAO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// render Cube
	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

unsigned int quadVAO = 0;
unsigned int quadVBO;
void render_quad()
{
	if (quadVAO == 0)
	{
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		// setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

void draw_outline(Model& outlineModel, Shader& outlineShader, glm::vec3 scale) {
	glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
	glStencilMask(0x00); //disable writing to the stencil buffer
	glDisable(GL_DEPTH_TEST);
	draw_scaled(outlineModel, outlineShader, scale);
	glStencilMask(0xFF);
	glStencilFunc(GL_ALWAYS, 1, 0XFF);
	glEnable(GL_DEPTH_TEST);
}

std::map<float, glm::vec3> get_sorted_position_map(std::vector<glm::vec3> positions) {
	std::map<float, glm::vec3> sorted;
	for (size_t i = 0; i < positions.size(); i++) {
		float distance = glm::length(state.camera.position - positions[i]);
		sorted[distance] = positions[i];
	}
	return sorted;
}

/* Organizational functions */

static void draw_scaled(Model& scaledModel, Shader& shader, glm::vec3 scale) {
	shader.use();
	glm::mat4 projection = glm::perspective(glm::radians(state.camera.zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
	glm::mat4 view = state.camera.getViewMatrix();
	shader.setMat4("projection", projection);
	shader.setMat4("view", view);
	// render the loaded model
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
	model = glm::scale(model, scale);
	shader.setMat4("model", model);

	scaledModel.draw(shader);
}

static void draw_depth(Model& depthModel, Shader &depthShader) {
	depthShader.use();
	glm::mat4 projection = glm::perspective(glm::radians(state.camera.zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
	glm::mat4 view = state.camera.getViewMatrix();
	depthShader.setMat4("projection", projection);
	depthShader.setMat4("view", view);
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); // translate it down so it's at the center of the scene
	model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));	// it's a bit too big for our scene, so scale it down
	depthShader.setMat4("model", model);

	depthModel.draw(depthShader);
}

/**** Callback Function Definitions ****/

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	if (state.firstMouse) {
		state.lastX = xpos;
		state.lastY = ypos;
		state.firstMouse = false;
	}
	float xoffset = xpos - state.lastX;
	float yoffset = state.lastY - ypos; // reversed since y-coordinates range from bottom to top
	state.lastX = xpos;
	state.lastY = ypos;
	state.camera.processMouseMovement(xoffset, yoffset, true);
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	state.camera.processMouseScroll(yoffset);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window, State::GameState& state) {
	if (state.inputManager.quit.press(window)) {
		glfwSetWindowShouldClose(window, true);
	}
	if (state.inputManager.toggle.press(window))
		state.toggleRenderGBuffer();

	if (state.inputManager.up.isPressed(window))
		state.moveCameraFore();
	if (state.inputManager.down.isPressed(window))
		state.moveCameraAft();
	if (state.inputManager.left.isPressed(window))
		state.moveCameraLeft();
	if (state.inputManager.right.isPressed(window))
		state.moveCameraRight();
	if (state.inputManager.opt1.isPressed(window)) {
		state.moveCameraUp();
	}
	if (state.inputManager.opt2.isPressed(window)) {
		state.moveCameraDown();
	}
	if (state.inputManager.action.press(window)) {
		state.toggleShadows();
	}

	state.inputManager.reset(window);
}