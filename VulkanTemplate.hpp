#ifndef _VULKAN_TEMPLATE_HEADER_
#define _VULKAN_TEMPLATE_HEADER_

#include <string>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <vector>
#include <optional>
#include <set>
#include <algorithm>
#include <fstream>
#include <array>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <stack>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_GTC_matrix_transform

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "imgui/imgui.h"
#include "imgui/imconfig.h"
#include "imgui/imgui_impl_vulkan.h"
#include "imgui/imgui_impl_glfw.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyObjLoader/tiny_obj_loader.h>

const std::string MODEL_PATH = "models/viking_room.obj";
const std::string TEXTURE_PATH = "textures/viking_room.png";

#ifdef _DEBUG
constexpr bool enableValidationLayers = true;
#else
constexpr bool enableValidationLayers = false;
#endif

#include "ValidationLayers.hpp"

static void check_vk_result(VkResult err) {
	if (err == 0)
		return;
	fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
	if (err < 0)
		abort();
}

namespace VkApplication{

	constexpr int MAX_FRAMES_IN_FLIGHT = 2;

	const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME
	};

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete() {
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

struct Vertex {
	glm::vec3 color;
	glm::vec3 vertexNormal;
	glm::vec3 pos;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(4);

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, color);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, vertexNormal);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, pos);

		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	}

	bool operator==(const Vertex& other) const {
		return pos == other.pos && color == other.color && vertexNormal == other.vertexNormal;
	}
};

// Per-instance data block
struct InstanceData {
	glm::vec3 pos;
	glm::vec3 rot;
	uint32_t texIndex;
	glm::vec3 instanceColor;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 1;
		bindingDescription.stride = sizeof(InstanceData);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

		return bindingDescription;
	}

	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(4);

		attributeDescriptions[0].binding = 1;
		attributeDescriptions[0].location = 4;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(InstanceData, pos);

		attributeDescriptions[1].binding = 1;
		attributeDescriptions[1].location = 5;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(InstanceData, rot);

		attributeDescriptions[2].binding = 1;
		attributeDescriptions[2].location = 6;
		attributeDescriptions[2].format = VK_FORMAT_R32_SINT;
		attributeDescriptions[2].offset = offsetof(InstanceData, texIndex);

		attributeDescriptions[3].binding = 1;
		attributeDescriptions[3].location = 7;
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(InstanceData, instanceColor);

		return attributeDescriptions;
	}

	bool operator==(const InstanceData& other) const {
		return pos == other.pos && rot == other.rot && texIndex == other.texIndex && instanceColor == other.instanceColor;
	}
};

/*
namespace std {
	template<> struct hash<VkApplication::Vertex> {
		size_t operator()(VkApplication::Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1);
		}
	};
}
*/

struct UniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 normalMatrix;
	glm::vec4 lightPos;
};

struct UniformFragmentObject {
	glm::vec4 Ambient;
	glm::vec4 LightColor;
	float Reflectivity;
	float Strength;
	glm::vec4 EyeDirection;
	float ConstantAttenuation;
	float LinearAttenuation;
	float QuadraticAttenuation;
	glm::mat4 viewMatrix;
	glm::mat4 eyeViewMatrix;
};

struct PushConstants {
	int useReflectionSampler;
};

struct {
	VkPipeline ground;
	VkPipeline cube;
	VkPipeline mirror;
} pipelines;

struct KeyControls {
	bool kickParticle = false;
};

class MainVulkApplication {

	friend void mainLoop(VkApplication::MainVulkApplication*);
	friend void updateUniformBuffer(MainVulkApplication*);
	friend void loadInitialVariables(MainVulkApplication*);

protected:
	MainVulkApplication() {}
	~MainVulkApplication() { cleanup(); }
	
public:

	MainVulkApplication(MainVulkApplication& other) = delete;
	void operator=(const MainVulkApplication&) = delete;

	static MainVulkApplication* GetInstance();

	void setup(std::string appName = "") {
		initWindow();
		initVulkan(appName);
	}


	void cleanupApp() {
		cleanup();
	}

private:

	static MainVulkApplication* pinstance_;

	size_t WIDTH = 800;
	size_t HEIGHT = 600;
	
	GLFWwindow* window;
	ImGui_ImplVulkanH_Window imgui_window;
	KeyControls keyControl;

	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkSurfaceKHR surface;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;

	VkQueue graphicsQueue;
	VkQueue presentQueue;

	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;

	VkRenderPass renderPass;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	VkCommandPool commandPool;

	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;

	std::vector<Vertex> vertices;

	std::vector<uint32_t> indices;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	size_t dynamicAlignment;
	size_t bufferDynamicSize;

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;

	std::vector<VkBuffer> uniformFragBuffers;
	std::vector<VkDeviceMemory> uniformFragBuffersMemory;

	std::vector<VkBuffer> uniformDynamicBuffers;
	std::vector<VkDeviceMemory> uniformDynamicBuffersMemory;

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	UniformBufferObject ubo;
	UniformFragmentObject ufo;

	std::vector<VkCommandBuffer> commandBuffers;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;

	size_t currentFrame = 0;

	bool framebufferResized = false;

	//FUNCTIONS
	void createInstance(std::string appName);
	//Create the viewport
	void initWindow();
	//Call back to re-buffer the viewport window
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
	void createSurface();
	//choose the correct GPU render device
	void pickPhysicalDevice();
	bool isDeviceSuitable(VkPhysicalDevice device);
	void createLogicalDevice();
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice );
	bool checkDeviceExtensionSupport(VkPhysicalDevice );

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& );
	void createSwapChain();
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR&);
	void createImageViews();
	VkImageView createImageView(VkImage, VkFormat, VkImageAspectFlags);
	void createImage(uint32_t, uint32_t,
		VkFormat, VkImageTiling, VkImageUsageFlags, VkMemoryPropertyFlags, VkImage&, VkDeviceMemory&);
	uint32_t findMemoryType(uint32_t, VkMemoryPropertyFlags);

	void createRenderPass();
	VkFormat findDepthFormat();
	VkFormat findSupportedFormat(const std::vector<VkFormat>&, VkImageTiling, VkFormatFeatureFlags);

	void createDescriptorSetLayout();
	void createGraphicsPipeline();
	VkShaderModule createShaderModule(const std::vector<char>&);
	void createCommandPool();
	void createDepthResources();
	void createFramebuffers();

	void createVertexBuffer();
	void createBuffer(VkDeviceSize, VkBufferUsageFlags,
		VkMemoryPropertyFlags, VkBuffer&, VkDeviceMemory&);
	void createIndexBuffer();
	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSets();
	
	void createImguiContext();
	void render_gui();
	void drawImgFrame(VkCommandBuffer& );

	void copyBuffer(VkBuffer, VkBuffer, VkDeviceSize);
	void createCommandBuffers();
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer);

	void createSyncObjects();
	void drawFrame();
	void updateUniformBuffer(uint32_t );
	void recreateSwapChain();
	void cleanupSwapChain();

	void loadModel();
	void createTextureImage();
	void copyBufferToImage(VkBuffer , VkImage , uint32_t , uint32_t );
	void transitionImageLayout(VkImage, VkFormat, VkImageLayout, VkImageLayout);
	void createTextureImageView();
	void createTextureSampler();


	void initVulkan(std::string appName ) {

		createInstance(appName);

		setupDebugMessenger();

		createSurface();
		pickPhysicalDevice();
		
		createLogicalDevice();
		createSwapChain();
		createImageViews();
		createRenderPass();
		createDescriptorSetLayout();
		createGraphicsPipeline();
		createCommandPool();
		createDepthResources();
		createFramebuffers();
		createTextureImage();
		createTextureImageView();
		createTextureSampler();

		loadModel();
		createVertexBuffer();
		createIndexBuffer();
		createUniformBuffers();
		createDescriptorPool();
		//createDescriptorSets();
		createImguiContext();
		createCommandBuffers();
		createSyncObjects();
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
			drawFrame();
		}

		vkDeviceWaitIdle(device);
	}

	void cleanup() {
		cleanupSwapChain();

		vkDestroySampler(device, textureSampler, nullptr);
		vkDestroyImageView(device, textureImageView, nullptr);

		vkDestroyImage(device, textureImage, nullptr);
		vkFreeMemory(device, textureImageMemory, nullptr);

		vkDestroyDescriptorPool(device, descriptorPool, nullptr);

		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

		for (size_t i = 0; i < swapChainImages.size(); i++) {
			vkDestroyBuffer(device, uniformBuffers[i], nullptr);
			vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
		}

		vkDestroyBuffer(device, indexBuffer, nullptr);
		vkFreeMemory(device, indexBufferMemory, nullptr);

		vkDestroyBuffer(device, vertexBuffer, nullptr);
		vkFreeMemory(device, vertexBufferMemory, nullptr);

		
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(device, inFlightFences[i], nullptr);
		}

		vkDestroyCommandPool(device, commandPool, nullptr);

		vkDestroyDevice(device, nullptr);

		if (enableValidationLayers) {
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}

		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);

		glfwDestroyWindow(window);

		glfwTerminate();
	}

	void setupDebugMessenger() {
		if (!enableValidationLayers) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
		populateDebugMessengerCreateInfo(createInfo);

		if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug messenger!");
		}
	}
};
}

namespace std {
	template<> struct hash<VkApplication::Vertex> {
		size_t operator()(VkApplication::Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}

#include "VulkanDescriptor.hpp"
#include "VulkanInstance.hpp"
#include "VulkanDevice.hpp"
#include "VulkanWindow.hpp"
#include "VulkanSwapchain.hpp"
#include "VulkanDraw.hpp"
#include "VulkanRenderSettings.hpp"
#include "VulkanSync.hpp"
#include "VulkanGeometry.hpp"
#include "VulkanTexture.hpp"
#include "VulkanImgui.hpp"

#endif