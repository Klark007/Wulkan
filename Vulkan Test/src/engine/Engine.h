#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "CameraController.h"

class Engine
{
public:
	Engine(GLFWwindow* window, std::shared_ptr<Camera> camera);
	void run();
private:
	GLFWwindow* window;

// TODO: remove public update, draw & late_update
public: 
	void update();
	void draw();
	void late_update(); // executed after draw
// TODO: remove public update, draw & late_update

public:
	CameraController camera_controller;
};

