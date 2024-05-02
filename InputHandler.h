#ifndef  __INPUTHANDLER_H__
#define  __INPUTHANDLER_H__

#include <stdlib.h>
#include <iostream>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

bool stopAnimation = true;
bool selectMode = false;
bool motionFlying = false;
bool motionMoving = false;
bool motionTurning = false;
bool movingMouse = false;

bool lbutton_down = false;
bool rbutton_down = false;

bool firstMouse = true;
double lastX = 0.0, lastY = 0.0;
float sensitivity = 0.01f;

bool forwardMovement = true;

double theta = 0.0;
double phi = 0.0;
double thetaRotation = 0.0;
double lastThetaRotation = 0.0;

GLfloat zdistance = 0.0f;

void readInput_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_W) {
		if (action == GLFW_PRESS) {
			forwardMovement = true;
			motionMoving = true;
		}
		else if (action == GLFW_RELEASE) {
			forwardMovement = false;
			motionMoving = false;
		}
	}
	else if (key == GLFW_KEY_S && action == GLFW_PRESS) {
		forwardMovement = false;
		motionMoving = true;
	}
	else if (key == GLFW_KEY_S && action == GLFW_RELEASE) {
		forwardMovement = false;
		motionMoving = false;
	}
	else if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
		exit(0);
	}
}

void mouse_cursor_callback(GLFWwindow* window, double xpos, double ypos) {
	if (firstMouse) {
		lastX = xpos; lastY = ypos; firstMouse = false;
	}
	movingMouse = true;
	double xOffset = xpos - lastX;
	double yOffset = lastY - ypos;
	xOffset *= sensitivity;
	yOffset *= sensitivity;

	if (lbutton_down) {
		theta += xOffset; phi += yOffset;
		phi = std::clamp(phi, 0.1, glm::pi<float>() - .1);
		lastX = xpos; lastY = ypos;
	}
	else if (rbutton_down) {
		
		if (abs(lastThetaRotation - xOffset) != 0.0)
			thetaRotation = -xOffset;
		else thetaRotation = 0.0;
		lastThetaRotation = thetaRotation;
		//std::cout << thetaRotation << std::endl;
	}
	else {
		firstMouse = true; thetaRotation = 0.0; 
	}

	lastX = xpos; lastY = ypos;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {

	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (GLFW_PRESS == action) {
			lbutton_down = true;
			motionFlying = true;
		}
		else if (GLFW_RELEASE == action) {
			lbutton_down = false;
			motionFlying = false;
		}
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
		if (GLFW_PRESS == action) {
			rbutton_down = true;
			motionTurning = true;
		}
		else if (GLFW_RELEASE == action) {
			rbutton_down = false;
			motionTurning = false;
		}

	}
}
#endif 