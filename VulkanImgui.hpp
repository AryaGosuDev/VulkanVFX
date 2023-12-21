#ifndef __VK_IMGUI_HPP__
#define __VK_IMGUI_HPP__

namespace VkApplication {

	VkSampler imgui_sampler;
	VkBuffer imgui_vertexBuffer;
	VkBuffer imgui_indexBuffer;
	VkDeviceMemory imgui_vertexBufferMemory;
	VkDeviceMemory imgui_indexBufferMemory;
	int32_t imgui_vertexCount = 0;
	int32_t imgui_indexCount = 0;
	VkDeviceMemory imgui_fontMemory = VK_NULL_HANDLE;
	VkImage imgui_fontImage = VK_NULL_HANDLE;
	VkImageView imgui_fontView = VK_NULL_HANDLE;
	VkPipelineCache imgui_pipelineCache;
	VkPipelineLayout imgui_pipelineLayout;
	VkPipeline imgui_pipeline;
	VkDescriptorPool imgui_descriptorPool;
	VkDescriptorSetLayout imgui_descriptorSetLayout;
	VkDescriptorSet imgui_descriptorSet;
	ImGuiStyle vulkanStyle;
	int selectedStyle = 0;

	struct UISettings {
		bool displayModels = true;
		bool displayLogos = true;
		bool displayBackground = true;
		bool animateLight = false;
		float lightSpeed = 0.25f;
		std::array<float, 50> frameTimes{};
		float frameTimeMin = 9999.0f, frameTimeMax = 0.0f;
		float lightTimer = 0.0f;
	} uiSettings;

	struct imgui_PushConstBlock {
		glm::vec2 scale;
		glm::vec2 translate;
	} imgui_pushConstBlock;

	void MainVulkApplication::createImguiContext() {

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		
		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsLight();

		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value() };
		
		// Create font texture
		unsigned char* fontData;
		int texWidth, texHeight;
		io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
		VkDeviceSize uploadSize = texWidth * texHeight * 4 * sizeof(char);
		//imgui_window.Surface = surface;
		// Create the Render Pass
		{
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);
			VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);

			VkAttachmentDescription colorattachment = {};
			//attachment.format = imgui_window.SurfaceFormat.format;
			colorattachment.format = surfaceFormat.format;
			colorattachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorattachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorattachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorattachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorattachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			//colorattachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			colorattachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorattachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			VkAttachmentReference color_attachment = {};
			color_attachment.attachment = 0;
			color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			/*
			VkAttachmentDescription depthAttachment = {};
			depthAttachment.format = VK_FORMAT_UNDEFINED;
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			*/
			VkAttachmentDescription depthAttachment{};
			depthAttachment.format = findDepthFormat();
			depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkAttachmentReference depthAttachmentRef = {};
			//depthAttachmentRef.attachment = VK_ATTACHMENT_UNUSED;  // Exclude depth attachment
			depthAttachmentRef.attachment = 1;
			depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			
			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &color_attachment;
			subpass.pDepthStencilAttachment = &depthAttachmentRef;

			VkSubpassDependency dependency = {};
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0;
			dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.srcAccessMask = 0;
			dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			std::array<VkAttachmentDescription, 2> attachments = { colorattachment, depthAttachment };
			VkRenderPassCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			info.attachmentCount = 2;
			info.pAttachments = attachments.data();
			info.subpassCount = 1;
			info.pSubpasses = &subpass;
			info.dependencyCount = 1;
			info.pDependencies = &dependency;

			check_vk_result(vkCreateRenderPass(device, &info, NULL, &imgui_window.RenderPass));
			// We do not create a pipeline by default as this is also used by examples' main.cpp,
			// but secondary viewport in multi-viewport mode may want to create one with:
			//ImGui_ImplVulkan_CreatePipeline(device, allocator, VK_NULL_HANDLE, wd->RenderPass, VK_SAMPLE_COUNT_1_BIT, &wd->Pipeline, bd->Subpass);
		}
		 
		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForVulkan(window, true);
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = instance;
		init_info.PhysicalDevice = physicalDevice;
		init_info.Device = device;
		init_info.QueueFamily = queueFamilyIndices[0];
		init_info.Queue = graphicsQueue;
		init_info.PipelineCache = NULL;
		init_info.DescriptorPool = descriptorPool;
		init_info.Subpass = 0;
		init_info.MinImageCount = MAX_FRAMES_IN_FLIGHT;
		//init_info.ImageCount = imgui_window.ImageCount;
		init_info.ImageCount = MAX_FRAMES_IN_FLIGHT;
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		init_info.Allocator = NULL;
		init_info.CheckVkResultFn = check_vk_result;
		ImGui_ImplVulkan_Init(&init_info, imgui_window.RenderPass);

		{
			
			VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
			commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			commandBufferAllocateInfo.commandPool = commandPool;
			commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			commandBufferAllocateInfo.commandBufferCount = 1;

			VkCommandBufferAllocateInfo cmdBufAllocateInfo = commandBufferAllocateInfo;
			VkCommandBuffer cpycmdBuffer;
			check_vk_result(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &cpycmdBuffer));
			//VkCommandBufferBeginInfo cmdBufInfo = Initializers::commandBufferBeginInfo();
			//check_vk_result(vkBeginCommandBuffer(cpycmdBuffer, &cmdBufInfo));
			
			//check_vk_result(vkResetCommandPool(device, command_pool, 0));
			VkCommandBufferBeginInfo begin_info = {};
			begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			check_vk_result(vkBeginCommandBuffer(cpycmdBuffer, &begin_info));

			ImGui_ImplVulkan_CreateFontsTexture(cpycmdBuffer);
			 
			VkSubmitInfo end_info = {};
			end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			end_info.commandBufferCount = 1;
			end_info.pCommandBuffers = &cpycmdBuffer;
			check_vk_result(vkEndCommandBuffer(cpycmdBuffer));
			check_vk_result(vkQueueSubmit(graphicsQueue, 1, &end_info, VK_NULL_HANDLE));

			check_vk_result(vkDeviceWaitIdle(device));
			ImGui_ImplVulkan_DestroyFontUploadObjects();
		}
	}

	void MainVulkApplication::drawImgFrame( VkCommandBuffer & commandBuffer) {

		render_gui();

		VkRenderPassBeginInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		info.renderPass = imgui_window.RenderPass;
		info.framebuffer = swapChainFramebuffers[currentFrame];
		info.renderArea.extent = swapChainExtent;
		info.clearValueCount = 1;

		std::array<VkClearValue, 1> clearValues{};
		clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.0f };
		info.clearValueCount = static_cast<uint32_t>(clearValues.size());
		info.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

		vkCmdEndRenderPass(commandBuffer);

	}

	void MainVulkApplication::render_gui() {

		// Start the Dear ImGui frame
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

		bool show_demo_window = true;
		bool show_another_window = false;

		{
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

			ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
			ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
			ImGui::Checkbox("Another Window", &show_another_window);

			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
			ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

			if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
			//ImGui::End();
		}

		// 3. Show another simple window.
		if (show_another_window)
		{
			ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
			ImGui::Text("Hello from another window!");
			if (ImGui::Button("Close Me"))
				show_another_window = false;
			//ImGui::End();
		}
		ImGui::End();
		// Rendering
		ImGui::Render();
		/*
		ImDrawData* draw_data = ImGui::GetDrawData(); draw_data->DisplaySize.x = 100; draw_data->DisplaySize.y = 100;
		const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
		if (!is_minimized) {
			imgui_window.ClearValue.color.float32[0] = clear_color.x * clear_color.w;
			imgui_window.ClearValue.color.float32[1] = clear_color.y * clear_color.w;
			imgui_window.ClearValue.color.float32[2] = clear_color.z * clear_color.w;
			imgui_window.ClearValue.color.float32[3] = clear_color.w;
			
		}
		*/

	}
	
	// TODO : Destroy imgui and pool
}
#endif