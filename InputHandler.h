#ifndef  __INPUTHANDLER_H__
#define  __INPUTHANDLER_H__

#include <stdlib.h>
#include <iostream>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

bool stopAnimation = true;
int motionMode = 0;
bool selectMode = false;
bool motionFlying = false;
bool kick = false;
double startX = 0;
double startY = 0;

bool lbutton_down = false;

double pointx;
double pointy;

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
	if (key == GLFW_KEY_S && action == GLFW_PRESS) {
		stopAnimation = !stopAnimation;
	}
	else if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
		//    kill( getpid(), SIGHUP );
		exit(0);
	}

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

	else if (key == GLFW_KEY_M && action == GLFW_PRESS) {
		motionMode++;
		if (motionMode == 3) motionMode = 0;

	}
	else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
		kick = true;
	}
}

void mouse_cursor_callback(GLFWwindow* window, double xpos, double ypos) {
	pointx = xpos;
	pointy = ypos;
	//std::cout << motionMode << std::endl;

	if (lbutton_down && motionMode == 0) {

		phi += (xpos - startX) / 100.0 ;
		startX = xpos;
		theta += (ypos - startY) / 100.0 ;
		startY = ypos;
		//std::cout << phi << std::endl;
	}
	else if (lbutton_down && motionMode == 1) {
		selectMode = true;
	}
	else if (motionMode == 1) {
		selectMode = false;
	}
}
 
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {

	if (button == GLFW_MOUSE_BUTTON_LEFT && motionMode == 0 ) {
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