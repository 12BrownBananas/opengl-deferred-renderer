#include <glm/glm.hpp>
#include <shader_util.h>

/* Definitions derived from http://devernay.free.fr/cours/opengl/materials.html */

namespace Materials {
	const Material EMERALD = Material(glm::vec3(0.07568f, 0.61424f, 0.07568), glm::vec3(0.633f, 0.727811f, 0.633f), 76.8f);
	const Material JADE = Material(glm::vec3(0.54f, 0.89f, 0.63f), glm::vec3(0.316228f, 0.316228f, 0.316228f), 1.28f);
	const Material OBSIDIAN = Material(glm::vec3(0.18275f, 0.17f, 0.22525f), glm::vec3(0.332741f, 0.328634f, 0.346435f), 38.4f);
	const Material PEARL = Material(glm::vec3(1.0f, 0.829f, 0.829f), glm::vec3(0.296648f, 0.296648f, 0.296648f), 11.264f);
	const Material RUBY = Material(glm::vec3(0.61424f, 0.04136f, 0.04136f), glm::vec3(0.727811f, 0.626959f, 0.626959f), 76.8f);
	const Material TURQUOISE = Material(glm::vec3(0.396f, 0.74151f, 0.69102f), glm::vec3(0.297254f, 0.30829f, 0.306678f), 12.8f);
	const Material BRASS = Material(glm::vec3(0.780392f, 0.568627f, 0.113725f), glm::vec3(0.992157f, 0.941176f, 0.807843f), 27.89743616f);
	const Material BRONZE = Material(glm::vec3(0.714f, 0.4284f, 0.18144f), glm::vec3(0.393548f, 0.271906f, 0.166721f), 25.6f);
	const Material CHROME = Material(glm::vec3(0.4f, 0.4f, 0.4f), glm::vec3(0.774597f, 0.774597f, 0.774597f), 76.8f);
	const Material COPPER = Material(glm::vec3(0.7038f, 0.27048f, 0.0828f), glm::vec3(0.256777f, 0.137622f, 0.086014f), 12.8f);
	const Material GOLD = Material(glm::vec3(0.75164f, 0.60648f, 0.22648f), glm::vec3(0.628281f, 0.555802f, 0.366065f), 51.2f);
	const Material SILVER = Material(glm::vec3(0.50754f, 0.50754f, 0.50754f), glm::vec3(0.508273f, 0.508273f, 0.508273f), 51.2f);
	const Material BLACK_PLASTIC = Material(glm::vec3(0.01f, 0.01f, 0.01f), glm::vec3(0.50f, 0.50f, 0.50f), 32.0f);
	const Material CYAN_PLASTIC = Material(glm::vec3(0.0f, 0.50980392f, 0.50980392f), glm::vec3(0.50196078f, 0.50196078f, 0.50196078f), 32.0f);
	const Material GREEN_PLASTIC = Material(glm::vec3(0.1f, 0.35f, 0.1f), glm::vec3(0.45f, 0.55f, 0.45f), 32.0f);
	const Material RED_PLASTIC = Material(glm::vec3(0.5f, 0.0f, 0.0f), glm::vec3(0.7f, 0.6f, 0.6f), 32.0f);
	const Material WHITE_PLASTIC = Material(glm::vec3(0.55f, 0.55f, 0.55f), glm::vec3(0.70f, 0.70f, 0.70f), 32.0f);
	const Material YELLOW_PLASTIC = Material(glm::vec3(0.5f, 0.5f, 0.0f), glm::vec3(0.60f, 0.60f, 0.50f), 32.0f);
	const Material BLACK_RUBBER = Material(glm::vec3(0.01f, 0.01f, 0.01f), glm::vec3(0.4f, 0.4f, 0.4f), 10.0f);
	const Material CYAN_RUBBER = Material(glm::vec3(0.4f, 0.5f, 0.5f), glm::vec3(0.04f, 0.7f, 0.7f), 10.0f);
	const Material GREEN_RUBBER = Material(glm::vec3(0.4f, 0.5f, 0.4f), glm::vec3(0.04f, 0.7f, 0.04f), 10.0f);
	const Material RED_RUBBER = Material(glm::vec3(0.5f, 0.4f, 0.4f), glm::vec3(0.7f, 0.04f, 0.04f), 10.0f);
	const Material WHITE_RUBBER = Material(glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(0.7f, 0.7f, 0.7f), 10.0f);
	const Material YELLOW_RUBBER = Material(glm::vec3(0.5f, 0.5f, 0.4f), glm::vec3(0.7f, 0.7f, 0.04f), 10.0f);
	const Material DEFAULT = Material(glm::vec3(1.0), glm::vec3(0.5f), 64.0f);
}