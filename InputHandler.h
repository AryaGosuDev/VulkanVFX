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
bool kick = false;
double startX = 0;
double startY = 0; 

bool lbutton_down = false;

bool firstMouse = true;
double lastX = 0.0, lastY = 0.0;
float sensitivity = 0.1f;

double pointx;
double pointy;
bool forwardMovement = true;

bool activateTrans = false;

float lightPositionx = 0.0f;
float lightPositionz = 0.0f;
float lightPositiony = 0.0f;
float lightPositionIncrement = 0.7f;
char changeLightPos[3] = {};
double theta = 0.0;
double phi = 0.0;

GLfloat zdistance = 0.0f;

void readInput_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {

	if (key == GLFW_KEY_W && action == GLFW_PRESS) {
		forwardMovement = true;
		motionMoving = true;
	}
	else if (key == GLFW_KEY_S && action == GLFW_PRESS) {
		forwardMovement = false;
		motionMoving = true;
	}

	else if (key == GLFW_KEY_Q && action == GLFW_PRESS) 
		exit(0);
	
	else if (key == GLFW_KEY_E && action == GLFW_PRESS) {
		lightPositiony = lightPositionIncrement;
		changeLightPos[1] = 1;
	}

	else if (key == GLFW_KEY_D && action == GLFW_PRESS) {
		lightPositiony = -lightPositionIncrement;
		changeLightPos[1] = 1;
	}

	else if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {
		lightPositionx = lightPositionIncrement;
		changeLightPos[0] = 1;
	}

	else if (key == GLFW_KEY_LEFT && action == GLFW_PRESS) {
		lightPositionx = -lightPositionIncrement;
		changeLightPos[0] = 1;
	}

	else if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) {
		lightPositionz = -lightPositionIncrement;
		changeLightPos[2] = 1;
	}

	else if (key == GLFW_KEY_UP && action == GLFW_PRESS) {
		lightPositionz = lightPositionIncrement;
		changeLightPos[2] = 1;
	}
}

void mouse_cursor_callback(GLFWwindow* window, double xpos, double ypos) {
	if (firstMouse) {
		lastX = xpos; lastY = ypos; firstMouse = false;
	}

	if (lbutton_down) {

		double xOffset = xpos - lastX; double yOffset = lastY - ypos; 
		xOffset *= sensitivity; yOffset *= sensitivity;

		phi += xOffset; theta += yOffset;
		// clamp to prevent flip
		theta = std::max(std::min(theta, 89.0), -89.0);
		lastX = xpos; lastY = ypos;
	}
	else  firstMouse = true; 
}
 
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {

	if (button == GLFW_MOUSE_BUTTON_LEFT ) {
		if (GLFW_PRESS == action) {
			lbutton_down = true;
			startX = pointx;
			startY = pointy;	 
			motionFlying = true;
		}
		else if (GLFW_RELEASE == action) {
			lbutton_down = false;
			motionFlying = false;
		}
	}
}
#endif 