#define _USE_MATH_DEFINES
#include <cmath>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>

#include <glm/gtx/vector_query.hpp>

#include "Geometry.h"
#include "GLDebug.h"
#include "Log.h"
#include "ShaderProgram.h"
#include "Shader.h"
#include "Texture.h"
#include "Window.h"
#include "imagebuffer.h"
#include "RayTrace.h"
#include "Scene.h"
#include "Lighting.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"


const float PI = 3.14159265359;

glm::vec3 InverseReflection(glm::vec3 a) {

	return glm::vec3(1 - a.x, 1 - a.y, 1 - a.z);
}

int hasIntersection(Scene const &scene, Ray ray, int skipID){
	for (auto &shape : scene.shapesInScene) {
		Intersection tmp = shape->getIntersection(ray);
		if(
			shape->id != skipID
			&& tmp.numberOfIntersections!=0
			&& glm::distance(tmp.point, ray.origin) > 0.00001
			&& glm::distance(tmp.point, ray.origin) < glm::distance(ray.origin, scene.lightPosition) - 0.01
		){
			return tmp.id;
		}
	}
	return -1;
}

Intersection getClosestIntersection(Scene const &scene, Ray ray, int skipID){ //get the nearest
	Intersection closestIntersection;
	float min = std::numeric_limits<float>::max();
	for(auto &shape : scene.shapesInScene) {
		if(skipID == shape->id) {
			// Sometimes you need to skip certain shapes. Useful to
			// avoid self-intersection. ;)
			continue;
		}
		Intersection p = shape->getIntersection(ray);
		float distance = glm::distance(p.point, ray.origin);
		if(p.numberOfIntersections !=0 && distance < min){
			min = distance;
			closestIntersection = p;
		}
	}
	return closestIntersection;
}


glm::vec3 raytraceSingleRay(Scene const &scene, Ray const &ray, int level, int source_id) {
	Intersection result = getClosestIntersection(scene, ray, source_id); //find intersection

	glm::vec3 colour;
	PhongReflection phong;


	phong.ray = ray;
	phong.scene = scene;
	phong.material = result.material;
	phong.intersection = result;


	if(result.numberOfIntersections == 0) return glm::vec3(0, 0, 0); // black;

	glm::vec3 dir = result.point - scene.lightPosition;
	dir = normalize(dir);
	Ray lightray(scene.lightPosition, dir);

	Intersection result2 = getClosestIntersection(scene, lightray, 999);



	if (glm::distance(result.point, result2.point) > 0.05 && result.id != result2.id) {
		colour = InverseReflection(phong.material.reflectionStrength) * phong.Ia();
	}
	else {
		colour = InverseReflection(phong.material.reflectionStrength) * phong.I();
	}

	if (level > 1 && glm::dot(phong.material.reflectionStrength, phong.material.reflectionStrength) > 0.05) {
		dir = ray.direction - ((2 * glm::dot(result.normal, ray.direction)) * result.normal);
		Ray r(result.point, dir);

		colour = colour + phong.material.reflectionStrength * raytraceSingleRay(scene, r, level - 1, result.id);
	}
	else return colour;

	return colour;
}


	


struct RayAndPixel {
	Ray ray;
	int x;
	int y;
};

std::vector<RayAndPixel> getRaysForViewpoint(Scene const &scene, ImageBuffer &image, glm::vec3 viewPoint) {

	std::vector<RayAndPixel> rays;


	glm::vec3 origin(0.f);
	float thetaX = PI / 3;
	float thetaY = PI / 3;
	float stepX = thetaX / image.Width();
	float stepY = thetaY / image.Height();
	float i = -(thetaY / 2);
	float j = -(thetaX / 2);
	for (int x = 0; x < image.Width() ; x++) {
		for (int y = 0; y < image.Height(); y++) {
			glm::vec3 direction = normalize(vec3(j, i, -1));
			Ray r = Ray(origin, direction);
			rays.push_back({r, x, y});
			i += stepY;
		}
		i = -(thetaY / 2);
		j += stepX;

	}
	return rays;
}

void raytraceImage(Scene const &scene, ImageBuffer &image, glm::vec3 viewPoint) {
	// Reset the image to the current size of the screen.
	image.Initialize();

	// Get the set of rays to cast for this given image / viewpoint
	std::vector<RayAndPixel> rays = getRaysForViewpoint(scene, image, viewPoint);

	for (auto const & r : rays) {
		glm::vec3 color = raytraceSingleRay(scene, r.ray, 5, -1);
		image.SetPixel(r.x, r.y, color);
	}
}

// EXAMPLE CALLBACKS
class Assignment5 : public CallbackInterface {

public:
	Assignment5() {
		viewPoint = glm::vec3(0, 0, 0);
		scene = initScene1();
		raytraceImage(scene, outputImage, viewPoint);
	}

	virtual void keyCallback(int key, int scancode, int action, int mods) {
		if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
			shouldQuit = true;
		}

		if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
			scene = initScene1();
			raytraceImage(scene, outputImage, viewPoint);
		}

		if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
			scene = initScene2();
			raytraceImage(scene, outputImage, viewPoint);
		}
	}

	bool shouldQuit = false;

	ImageBuffer outputImage;
	Scene scene;
	glm::vec3 viewPoint;

};
// END EXAMPLES


int main() {
	Log::debug("Starting main");

	// WINDOW
	glfwInit();

	// Change your image/screensize here.
	int width = 800;
	int height= 800;
	Window window(width, height, "CPSC 453");

	GLDebug::enable();

	// CALLBACKS
	std::shared_ptr<Assignment5> a5 = std::make_shared<Assignment5>(); // can also update callbacks to new ones
	window.setCallbacks(a5); // can also update callbacks to new ones

	// RENDER LOOP
	while (!window.shouldClose() && !a5->shouldQuit) {
		glfwPollEvents();

		glEnable(GL_FRAMEBUFFER_SRGB);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		a5->outputImage.Render();

		window.swapBuffers();
	}


	// Save image to file:
	a5->outputImage.SaveToFile("foo.png");

	glfwTerminate();
	return 0;
}
