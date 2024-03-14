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
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>

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
#include <glm/ext.hpp>

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

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

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

namespace VkApplication {

	//const std::string WORLD_PATH = "models/city.obj";
	//const std::string WORLD_PATH = "models/city_reduced.obj";
	const std::string WORLD_PATH = "models/simpleCubes.obj";
	const std::string WORLD_PATH_LOWPOLY = "models/city_lowpoly.obj";
	const std::string AVATAR_PATH = "models/avatar.obj";
	const std::string TEXTURE_PATH = "";
	//const std::string WORLD_AABB_PATH = "models/output_AABB.csv";
	//const std::string WORLD_AABB_PATH = "models/output_AABB_reduced.csv";
	const std::string WORLD_AABB_PATH = "models/output_AABB_SimpleCubes.csv";
	const std::string GROUND_PATH = "models/ground.obj";
	constexpr int MAX_FRAMES_IN_FLIGHT = 2;

	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	//VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME
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

	struct Plane {
		glm::vec3 normal;
		float d;
	};

	struct Frustrum {
		Plane planes[6];
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

/*
namespace std {
	template<> struct hash<VkApplication::Vertex> {
		size_t operator()(VkApplication::Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1);
		}
	};
}
*/

	class join_threads {
		std::vector<std::thread>& threads;

	public:
		explicit join_threads(std::vector<std::thread>& _threads) : threads(_threads) {}
		~join_threads() {
			for (unsigned int i = 0; i < threads.size(); ++i) {
				if (threads[i].joinable()) {
					threads[i].join();
				}

			}
		}
	};

	template<class T>
	class threadsafe_queue
	{
	private:
		mutable std::mutex mut;
		std::queue<T> data_queue;
		std::condition_variable data_cond;
	public:
		threadsafe_queue() = default;

		void push(T new_value) {
			std::lock_guard<std::mutex> lk(mut);
			data_queue.push(std::move(new_value));
			data_cond.notify_one();
		}

		bool wait_and_pop(T& value) {
			std::unique_lock<std::mutex> lk(mut);
			data_cond.wait(lk, [this] {return !data_queue.empty(); });
			value = std::move(data_queue.front());
			data_queue.pop();
			return true;
		}

		std::shared_ptr<T> wait_and_pop() {
			std::unique_lock<std::mutex> lk(mut);
			data_cond.wait(lk, [this] {return !data_queue.empty(); });
			std::shared_ptr<T> res(std::make_shared<T>(std::move(data_queue.front())));
			data_queue.pop();
			return res;
		}

		bool try_pop(T& value) {
			//std::cout << "Trying to pop" << std::endl;
			std::lock_guard<std::mutex> lk(mut);
			if (data_queue.empty())
				return false;
			value = std::move(data_queue.front());
			data_queue.pop();
			return true;
		}

		std::shared_ptr<T> try_pop() {
			std::lock_guard<std::mutex> lk(mut);
			if (data_queue.empty())
				return std::shared_ptr<T>();
			std::shared_ptr<T> res(
				std::make_shared<T>(std::move(data_queue.front())));
			data_queue.pop();
			return res;
		}

		bool empty() const
		{
			std::lock_guard<std::mutex> lk(mut);
			return data_queue.empty();
		}
	};

	struct UniformBufferObject {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 normalMatrix;
		glm::vec4 lightPos;
		glm::mat4 orthoProj;
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
		VkPipeline albedo;
		VkPipeline normals;
		VkPipeline depthInfo;
		VkPipeline worldCoord;
	} GBufferPipelines;

	struct GbufferImageViews{
		VkImage AlbedoImage;
		VkImageView AlbedoImageView;
		VkDeviceMemory AlbedoImageMemory;

		VkImage DepthInfoImage;
		VkImageView DepthInfoImageView;
		VkDeviceMemory DepthInfoImageMemory;

		VkImage NormalsImage;
		VkImageView NormalsImageView;
		VkDeviceMemory NormalsImageMemory;

		VkImage WorldCoordImage;
		VkImageView WorldCoordImageView;
		VkDeviceMemory WorldCoordImageMemory;

		VkImage GBufferImageDepth;
		VkImageView GBufferImageDepthView;
		VkDeviceMemory GBufferImageDepthMemory;

	} ;

	struct Thread_pool_frustrum_culling;

	struct AABB {
		glm::vec3 min;
		glm::vec3 max;
		AABB() = default;
		AABB(glm::vec3 _v1, glm::vec3 _v2, glm::vec3 _v3, glm::vec3 _v4, glm::vec3 _v5, glm::vec3 _v6, glm::vec3 _v7, glm::vec3 _v8) {
			std::vector<glm::vec3> samples = { _v1, _v2, _v3 ,_v4 ,_v5 ,_v6 ,_v7 ,_v8 };
			min = _v1; max = _v1;
			for (int i = 1; i < 8; ++i) {
				min.x = std::min(min.x, samples[i].x);
				min.y = std::min(min.y, samples[i].y);
				min.z = std::min(min.z, samples[i].z);

				max.x = std::max(max.x, samples[i].x);
				max.y = std::max(max.y, samples[i].y);
				max.z = std::max(max.z, samples[i].z);
			}
		}
		AABB(glm::vec3 _min, glm::vec3 _max) : min(_min), max(_max) {}
	};

	struct QuadTreeNode {
		QuadTreeNode() = default;
		~QuadTreeNode() {
			children[0] = children[1] = children[2] = children[3] = NULL;
		}
		AABB box;
		bool isLeaf = false;
		std::vector<std::string> objectIDs;
		QuadTreeNode* children[4] = { NULL, NULL, NULL, NULL };
	};

	class QuadTree {

	public:
		QuadTree() = default;
		QuadTree(std::unordered_map<std::string, AABB>& objectAABB) {
			if (root == NULL) {
				root = new QuadTreeNode();
				root->box = constructTotalAABB(objectAABB);
				for (auto& v : objectAABB) root->objectIDs.push_back(v.first);
			}
			createTree(root, objectAABB);
		}
		QuadTree& operator = (QuadTree& other) {
			this->root = other.root;
		}
		QuadTree& operator = (QuadTree&& other) noexcept {
			if (this != &other) {
				this->root = other.root;
				other.root = NULL;
			}
			return *this;
		}
		~QuadTree() {}

		QuadTreeNode* returnRoot() { return root; }

		bool intersects(const AABB& a, const AABB& b) {
			return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
				(a.min.z <= b.max.z && a.max.z >= b.min.z);
		}

		void createTree(QuadTreeNode* node, std::unordered_map<std::string, AABB>& objectAABB) {
			if (node == NULL) return;
			//create quads
			if (node->objectIDs.size() > THRESHOLD) {
				//split aabb into 4 quads
				AABB& currentAABB = node->box;
				float midX = (currentAABB.max.x + currentAABB.min.x) / 2.0f;
				float midZ = (currentAABB.max.z + currentAABB.min.z) / 2.0f;
				node->children[0] = new QuadTreeNode(); node->children[1] = new QuadTreeNode();
				node->children[2] = new QuadTreeNode(); node->children[3] = new QuadTreeNode();
				// split box in 4 quadrants
				node->children[0]->box = AABB(currentAABB.min, glm::vec3(midX, currentAABB.max.y, midZ));
				node->children[1]->box = AABB(glm::vec3(midX, currentAABB.min.y, currentAABB.min.z),
					glm::vec3(currentAABB.max.x, currentAABB.max.y, midZ));
				node->children[2]->box = AABB(glm::vec3(currentAABB.min.x, currentAABB.min.y, midZ),
					glm::vec3(midX, currentAABB.max.y, currentAABB.max.z));
				node->children[3]->box = AABB(glm::vec3(midX, currentAABB.min.y, midZ),
					currentAABB.max);

				std::unordered_set<std::string> tempIDs(std::begin(node->objectIDs), std::end(node->objectIDs));

				for (size_t i = 0; i < 4; ++i) {

					for (auto& aabbObject : tempIDs) {
						if (intersects(node->children[i]->box, objectAABB[aabbObject])) {
							node->children[i]->objectIDs.push_back(aabbObject);

						}
					}
					// so if there are some aabb that are part of multiple aabbs, include them in both, leave commented
					//for (auto& v : objectsToAdd) tempIDs.erase(v);
				}

				for (size_t i = 0; i < 4; ++i) {

					if (node->children[i]->objectIDs.size() != 0)
						createTree(node->children[i], objectAABB);
					else if (node->children[i]->objectIDs.size() == 0) {
						for (int j = 0; j < 4; ++j) node->children[i]->children[j] = NULL;
						delete node->children[i]; node->children[i] = NULL;
					}
				}
				int numNull = 0;
				for (size_t i = 0; i < 4; ++i) {
					if (node->children[i] == NULL) numNull++;
				}
				if (numNull == 4) node->isLeaf = true;
			}
			else {
				node->isLeaf = true;
				for (int i = 0; i < 4; ++i) node->children[i] = NULL;
			}
		}

	private:
		QuadTreeNode* root = NULL;
		const int THRESHOLD = 10;

		AABB constructTotalAABB(const std::unordered_map<std::string, AABB>& objectAABB) {
			if (objectAABB.empty())
				return AABB{};

			auto iter = objectAABB.begin();
			glm::vec3 globalMin = iter->second.min;
			glm::vec3 globalMax = iter->second.max;

			for (const auto& kv : objectAABB) {
				globalMin.x = std::min(globalMin.x, kv.second.min.x);
				globalMin.y = std::min(globalMin.y, kv.second.min.y);
				globalMin.z = std::min(globalMin.z, kv.second.min.z);

				globalMax.x = std::max(globalMax.x, kv.second.max.x);
				globalMax.y = std::max(globalMax.y, kv.second.max.y);
				globalMax.z = std::max(globalMax.z, kv.second.max.z);
			}

			return AABB{ globalMin, globalMax };
		}
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

	size_t WIDTH = 1600;
	size_t HEIGHT = 1200;
	
	GLFWwindow* window;
	ImGui_ImplVulkanH_Window imgui_window;
	Assimp::Importer importer;

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
	std::vector<Vertex> vertices_lowpoly;
	std::vector<uint32_t> indices_lowpoly;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer vertexLowPolyBuffer;
	VkDeviceMemory vertexLowPolyBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;
	VkBuffer indexLowPolyBuffer;
	VkDeviceMemory indexLowPolyBufferMemory;

	std::vector<Vertex> vertices_ground;
	std::vector<uint32_t> indices_ground;
	VkBuffer vertexBuffer_ground;
	VkDeviceMemory vertexBufferMemory_ground;
	VkBuffer indexBuffer_ground;
	VkDeviceMemory indexBufferMemory_ground;

	std::vector<Vertex> vertices_avatar;
	std::vector<uint32_t> indices_avatar;
	VkBuffer vertexBuffer_avatar;
	VkDeviceMemory vertexBufferMemory_avatar;
	VkBuffer indexBuffer_avatar;
	VkDeviceMemory indexBufferMemory_avatar;
	
	std::unordered_map<std::string, std::pair<uint32_t, uint32_t>> objectHash;
	std::unordered_map<std::string, int> objectHash_lowpoly;
	std::unordered_map<std::string, AABB > objectAABB;
	std::unordered_set<std::string > objectsToRenderForFrame;

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;

	std::vector<VkBuffer> uniformFragBuffers;
	std::vector<VkDeviceMemory> uniformFragBuffersMemory;

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

	//***************************** GBUFFER VARIABLES ****************************
	
	std::vector<GbufferImageViews> gbufferImageViews;

	std::vector<VkFramebuffer> GBufferFramebuffer;
	VkDescriptorSetLayout descriptorSetLayoutGBuffer;
	VkRenderPass renderPassGBuffer;
	std::vector<VkCommandBuffer> GbufferCommandBuffer;
	VkPipeline GBufferPipeline;
	VkPipelineLayout GBufferPipelineLayout;
	std::vector<VkDescriptorSet> descriptorSetGBuffer;
	VkDescriptorPool GBufferDescriptorPool;

	std::vector<VkFence> GBufferFence;
	VkSemaphore renderStartSemaphoreGBuffer = 0;
	VkSemaphore renderCompleteSemaphoreGBuffer = 0;
	VkSemaphore gBufferCompleteSemaphore;

	QuadTree worldQuadTree;
	Thread_pool_frustrum_culling * FrustCullThreadPool;

	//***************************** LIGHT DEPTH MAP VARIABLES ****************************

	VkFramebuffer LightDepthFramebuffer;
	GbufferImageViews LightDepthImageViews;
	VkDescriptorSetLayout descriptorSetLayoutLightDepth;
	VkRenderPass renderPassLightDepth;
	VkCommandBuffer LightDepthCommandBuffer;
	VkPipeline LightDepthPipeline;
	VkPipelineLayout LightDepthPipelineLayout;
	VkDescriptorSet descriptorSetLightDepth;
	VkDescriptorPool LightDepthDescriptorPool;
	VkFence LightDepthFence;
	VkSemaphore LightDepthCompleteSemaphore;

	//**********************    FUNCTIONS *************************************
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
	void createDescriptorSetLayoutDebug();
	void createGraphicsPipeline();
	VkShaderModule createShaderModule(const std::vector<char>&);
	void createCommandPool();
	void createDepthResources();
	void createFramebuffers();

	void createVertexBuffer();
	void createVertexLowPolyBuffer();
	void createBuffer(VkDeviceSize, VkBufferUsageFlags,
		VkMemoryPropertyFlags, VkBuffer&, VkDeviceMemory&);
	void createIndexBuffer();
	void createGeometryBuffer(std::vector<Vertex>&, VkBuffer&, VkDeviceMemory&);
	void createIndexBuffer(std::vector<uint32_t>&, VkBuffer&, VkDeviceMemory&);
	void createIndexLowPolyBuffer();
	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSets();
	void createDescriptorSetsDebug();
	
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

	void GbufferRenderPipelineSetup();
	void createDescriptorSetGBuffer();
	void setupGBufferCommandBuffer();
	void GBufferDraw(uint32_t& imageIndex);
	void pruneGeo(const glm::mat4 proj, const glm::mat4 view, std::shared_ptr< const QuadTreeNode* const> _node);

	void LightDepthDraw();
	void LightDepthRenderPipelineSetup();
	
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
		//createDescriptorSetLayoutDebug();
		createGraphicsPipeline();
		createCommandPool();
		createDescriptorPool();

		createDepthResources();
		createFramebuffers();
		//createTextureImage();
		//createTextureImageView();
		createTextureSampler();

		GbufferRenderPipelineSetup();
		
		loadModel();
		createVertexBuffer();
		createIndexBuffer();
		createGeometryBuffer(vertices_ground, vertexBuffer_ground, vertexBufferMemory_ground);
		createIndexBuffer(indices_ground, indexBuffer_ground, indexBufferMemory_ground);
		createGeometryBuffer(vertices_avatar, vertexBuffer_avatar, vertexBufferMemory_avatar);
		createIndexBuffer(indices_avatar, indexBuffer_avatar, indexBufferMemory_avatar);
		//createVertexLowPolyBuffer();
		//createIndexLowPolyBuffer();
		createUniformBuffers();
		
		createDescriptorSets();
		//createDescriptorSetsDebug();
		createImguiContext();
		createCommandBuffers();
		createSyncObjects();
		
		createDescriptorSetGBuffer();
		setupGBufferCommandBuffer();
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
#include "VulkanGBuffer.hpp"
#include "VulkanLightDepth.hpp"
#include "VulkanImgui.hpp"

#endif