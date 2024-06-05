#include "VulkanTemplate.hpp"
#include "InputHandler.h"

using std::cout;
using std::endl;

VkApplication::MainVulkApplication* VkApplication::MainVulkApplication::pinstance_ = NULL;

VkApplication::MainVulkApplication* VkApplication::MainVulkApplication::GetInstance() {
	if (pinstance_ == nullptr)
		pinstance_ = new MainVulkApplication();
	return pinstance_;
}

namespace VkApplication {

	struct perspectiveData {
		float fieldOfView;
		float aspect;
		float nearPlane;
		float farPlane;
	};

	perspectiveData pD;

	// storage for matrices
	glm::mat4 viewMatrix;
	glm::mat4 eyeviewMatrix;
	glm::vec4 ambientLight(0.1f, 0.1f, 0.1f, 1.0f);
	glm::vec4 lightColor(0.8f, 0.8f, 0.8f, 1.0f);
	glm::vec4 LightPosition(0.0f, 5.0f, 0.0f, 1.0f);
	float Shininess = 1.1f;
	float Strength = 60.0f;
	glm::vec4 EyeDirection(0.1f, 1.0f, -3.0f, 1.0f);
	float ConstantAttenuation = 2.0f;
	float LinearAttenuation = 0.0f;
	float QuadraticAttenuation = 0.0f;
	glm::mat4 normalModelViewMatrix;

	glm::vec3 mainEyeLoc(0.0f, 1.0f, -5.0f);
	glm::vec3 initialCameraOffset(0.0f, 1.0f, -5.0f);
	glm::vec3 centerLoc(0.0f, 1.0f, 0.0f);
	glm::vec3 up(0.0f, 1.0f, 0.0f);
	glm::vec3 world_up(0.0f, 1.0f, 0.0f);
	float fov = glm::radians<float>(45.0f);
	glm::mat4 copyView;
	glm::vec3 avatarLocation = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 avatarLookingDir = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 initialavatarLookingDir = avatarLookingDir;
	float cameraDistance = 5.0f;
	float rotationSpeed = 0.1f;
	float movementSpeed = 0.02f;

	//need to load in the value of the uniform buffers for the frag and vert shader + load attributes
	void loadInitialVariables(VkApplication::MainVulkApplication* _app) {
		_app->ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		mainEyeLoc = avatarLocation + initialCameraOffset;
		centerLoc = glm::normalize(avatarLookingDir) * cameraDistance + mainEyeLoc;
		_app->ubo.view = glm::lookAt(mainEyeLoc, centerLoc, up);
		copyView = _app->ubo.view;
		//_app->ubo.proj = glm::perspective(fov, _app->swapChainExtent.width / (float)_app->swapChainExtent.height, 0.5f, 20.0f);
		_app->ubo.proj = glm::perspective(fov, _app->swapChainExtent.width / (float)_app->swapChainExtent.height, 0.5f, 40.0f);
		_app->ubo.proj[1][1] *= -1.0f;
		glm::mat3 viewMatrix3x3(_app->ubo.view * _app->ubo.model);
		_app->ubo.normalMatrix = glm::inverseTranspose(viewMatrix3x3);
		_app->ubo.lightPos = LightPosition;

		_app->ufo.Ambient = ambientLight;
		_app->ufo.LightColor = lightColor;
		_app->ufo.Reflectivity = Shininess;
		_app->ufo.Strength = Strength;
		_app->ufo.EyeDirection = glm::vec4(mainEyeLoc, 1.0f);
		_app->ufo.ConstantAttenuation = ConstantAttenuation;
		_app->ufo.LinearAttenuation = LinearAttenuation;
		_app->ufo.QuadraticAttenuation = QuadraticAttenuation;
		_app->ufo.viewMatrix = glm::inverseTranspose(viewMatrix3x3);;
		_app->ufo.eyeViewMatrix = glm::inverseTranspose(viewMatrix3x3);

		_app->ubo.orthoProj =	   glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 1.0f, 100.0f);
		_app->ubo.LightDepthView = glm::lookAt(glm::vec3(LightPosition), centerLoc, up);
	}
	/*
	void updateUniformBuffer(VkApplication::MainVulkApplication* _app) {

		if (motionFlying == true) {
			mainEyeLoc.x = (cameraDistance * float(sin(phi)) * float(cos(theta))) + avatarLocation.x;
			mainEyeLoc.y = (cameraDistance * float(cos(phi))) + avatarLocation.y;
			mainEyeLoc.z = (cameraDistance * float(sin(theta)) * float(sin(phi))) + avatarLocation.z;
			//centerLoc = avatarLocation + avatarLookingDir * cameraDistance;
			_app->ubo.view = glm::lookAt(mainEyeLoc, centerLoc, up);
		}

		if (motionTurning == true && movingMouse == true) {
			
			glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), (float)thetaRotation * rotationSpeed, world_up);
			avatarLookingDir = glm::vec3(rotationMatrix * glm::vec4(avatarLookingDir, 0.0f));
			avatarLookingDir = glm::normalize(avatarLookingDir);
			mainEyeLoc = (avatarLocation + -avatarLookingDir * cameraDistance) + glm::vec3(0.0f, 1.0f, 0.0f);
			centerLoc = avatarLookingDir * cameraDistance + mainEyeLoc;

			_app->ubo.view = glm::lookAt(mainEyeLoc, centerLoc, up);
			movingMouse = false;
		}

		if (forwardMovement == true && motionMoving == true) {
			if (glfwGetKey(_app->window, GLFW_KEY_W) == GLFW_PRESS) {
				avatarLocation += avatarLookingDir * 0.2f;  // Adjust movement speed as necessary
			}
			//avatarLocation += avatarLookingDir * 0.2;
			mainEyeLoc = avatarLocation + initialCameraOffset;
			centerLoc = glm::normalize(avatarLookingDir) * cameraDistance + mainEyeLoc;

			//motionMoving = false;
			_app->ubo.view = glm::lookAt(mainEyeLoc, centerLoc, up);
		}

		_app->avatarInfo.avatarPos = glm::translate(glm::mat4(1.0f), avatarLocation);
		_app->avatarInfo.rotation =  glm::rotate(glm::mat4(1.0f), (float)thetaRotation * rotationSpeed, world_up);

		_app->ubo.LightDepthView = glm::lookAt(glm::vec3(LightPosition), centerLoc, up);
	}
	*/
	void updateUniformBuffer(VkApplication::MainVulkApplication* _app) {
		// Continuous movement forward with 'W' key press.
		if (glfwGetKey(_app->window, GLFW_KEY_W) == GLFW_PRESS) {
			avatarLocation += avatarLookingDir * movementSpeed; // Move forward
			mainEyeLoc += avatarLookingDir * movementSpeed;
			centerLoc += avatarLookingDir * movementSpeed;
			_app->ubo.view = glm::lookAt(mainEyeLoc, centerLoc, up);
			motionMoving = true; // Acknowledge movement
		}
		else motionMoving = false; // Stop moving when 'W' is not pressed

		// Handling flying movement separately
		if (motionFlying == true) {
			mainEyeLoc.x = (cameraDistance * float(sin(phi)) * float(cos(theta))) + avatarLocation.x;
			mainEyeLoc.y = (cameraDistance * float(cos(phi))) + avatarLocation.y;
			mainEyeLoc.z = (cameraDistance * float(sin(theta)) * float(sin(phi))) + avatarLocation.z;
			//centerLoc = avatarLocation + avatarLookingDir * cameraDistance;
			_app->ubo.view = glm::lookAt(mainEyeLoc, centerLoc, up);
		}

		// Adjust rotation based on mouse input when the right button is down
		if (motionTurning && movingMouse) {
			glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), (float)thetaRotation * rotationSpeed, world_up);
			avatarLookingDir = glm::vec3(rotationMatrix * glm::vec4(avatarLookingDir, 0.0f));
			avatarLookingDir = glm::normalize(avatarLookingDir);
			mainEyeLoc = (avatarLocation - avatarLookingDir * cameraDistance) + glm::vec3(0.0f, 1.0f, 0.0f);
			centerLoc = avatarLookingDir * cameraDistance + mainEyeLoc;
			_app->ubo.view = glm::lookAt(mainEyeLoc, centerLoc, up);
			movingMouse = false; // Reset the flag after processing
		}

		// Update avatar transformation matrix
		_app->avatarInfo.avatarPos = glm::translate(glm::mat4(1.0f), avatarLocation);
		// Compute the angle of rotation using the dot product
		float angle = glm::acos(glm::dot(initialavatarLookingDir, avatarLookingDir));
		glm::vec3 sign = glm::cross(initialavatarLookingDir, avatarLookingDir);
		if ( sign.y < 0.0) _app->avatarInfo.rotation = glm::rotate(glm::mat4(1.0f), -angle, world_up);
		else _app->avatarInfo.rotation = glm::rotate(glm::mat4(1.0f), angle, world_up);
		
		// Update light depth view matrix for shadow mapping
		_app->ubo.LightDepthView = glm::lookAt(glm::vec3(LightPosition), centerLoc, up);
	}


	void mainLoop(VkApplication::MainVulkApplication* _app) {

		glfwSetKeyCallback(_app->window, readInput_callback);
		glfwSetCursorPosCallback(_app->window, mouse_cursor_callback);
		glfwSetMouseButtonCallback(_app->window, mouse_button_callback);

		int WindowRes;

		while (!(WindowRes = glfwWindowShouldClose(_app->window))) {
			glfwPollEvents();
			updateUniformBuffer(_app);
			_app->drawFrame();
		}
		vkDeviceWaitIdle(_app->device);
	}
}

int main() {

	VkApplication::MainVulkApplication* vkApp_ = VkApplication::MainVulkApplication::GetInstance();

	try {
		vkApp_->setup();
		loadInitialVariables(vkApp_);
		mainLoop(vkApp_);
		vkApp_->cleanupApp();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}