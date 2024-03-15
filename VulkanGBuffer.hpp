#ifndef __VULKAN_GBUFFER_HPP__
#define __VULKAN_GBUFFER_HPP__

namespace VkApplication {

	void renderObjectsIndexPass(std::unordered_set<std::string >& objectsToRenderForFrame, uint32_t& imageIndex, 
		std::vector<VkCommandBuffer>& GbufferCommandBuffer, uint32_t indices_size, std::unordered_map<std::string, std::pair<uint32_t, uint32_t>> & objectHash) {

		for (const auto& objectName : objectsToRenderForFrame) {

			auto& offsets = objectHash[objectName];
			uint32_t indexOffset = offsets.first;
			uint32_t indexCount = offsets.second ; // Adjust based on your actual structure

			vkCmdDrawIndexed(GbufferCommandBuffer[imageIndex], indexCount, 1, indexOffset, 0, 0);
		}
	}

	void renderDrawGround( uint32_t& imageIndex, std::vector<VkCommandBuffer>& GbufferCommandBuffer, 
		uint32_t indices_size, VkBuffer & mainVertexBuffer, VkBuffer & mainIndexBuffer  ) {

		VkBuffer vertexBuffers[] = { mainVertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(GbufferCommandBuffer[imageIndex], 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(GbufferCommandBuffer[imageIndex], mainVertexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(GbufferCommandBuffer[imageIndex], indices_size, 1, 0, 0, 0);
	}

	void renderDrawAvatar( uint32_t& imageIndex, std::vector<VkCommandBuffer>& GbufferCommandBuffer,
		uint32_t indices_size, VkBuffer& mainVertexBuffer, VkBuffer& mainIndexBuffer) {

		VkBuffer vertexBuffers[] = { mainVertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(GbufferCommandBuffer[imageIndex], 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(GbufferCommandBuffer[imageIndex], mainVertexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(GbufferCommandBuffer[imageIndex], indices_size, 1, 0, 0, 0);
	}

	void MainVulkApplication::GBufferDraw(uint32_t& imageIndex) {
		FrustCullThreadPool->start = true;
		auto childShared = std::make_shared< const QuadTreeNode* const>(worldQuadTree.returnRoot());
		FrustCullThreadPool->submit([=]() { pruneGeo(ubo.proj, ubo.view, childShared); });
		FrustCullThreadPool->waitUntilDone();
		//std::cout << currentFrame << std::endl;

		vkWaitForFences(device, 1, &GBufferFence[currentFrame], VK_TRUE, UINT64_MAX);

		// Start recording commands
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		check_vk_result(vkBeginCommandBuffer(GbufferCommandBuffer[imageIndex], &beginInfo));

		// Begin the picking render pass
		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPassGBuffer;
		renderPassInfo.framebuffer = GBufferFramebuffer[imageIndex];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = { swapChainExtent.width, swapChainExtent.height };  // Set this to the size of your picking framebuffer

		std::array<VkClearValue, 5> clearValues{};
		clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[1].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[2].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[3].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[4].depthStencil = { 1.0f, 0 };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(GbufferCommandBuffer[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		// albedo
		vkCmdBindPipeline(GbufferCommandBuffer[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, GBufferPipelines.albedo);
		VkBuffer vertexBuffers[] = { vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindDescriptorSets(GbufferCommandBuffer[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, GBufferPipelineLayout, 0, 1, &descriptorSetGBuffer[imageIndex], 0, nullptr);
		vkCmdBindVertexBuffers(GbufferCommandBuffer[imageIndex], 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(GbufferCommandBuffer[imageIndex], indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		
		renderObjectsIndexPass(objectsToRenderForFrame, imageIndex, GbufferCommandBuffer, static_cast<uint32_t>(indices.size()), objectHash);
		renderDrawGround(imageIndex, GbufferCommandBuffer, static_cast<uint32_t>(indices_ground.size()), vertexBuffer_ground, indexBuffer_ground);
		renderDrawAvatar(imageIndex, GbufferCommandBuffer, static_cast<uint32_t>(indices_avatar.size()), vertexBuffer_avatar, indexBuffer_avatar);
		
		vkCmdNextSubpass(GbufferCommandBuffer[imageIndex], VK_SUBPASS_CONTENTS_INLINE);

		//normals
		vkCmdBindPipeline(GbufferCommandBuffer[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, GBufferPipelines.normals);

		vkCmdBindDescriptorSets(GbufferCommandBuffer[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, GBufferPipelineLayout, 0, 1, &descriptorSetGBuffer[imageIndex], 0, nullptr);
		vkCmdBindVertexBuffers(GbufferCommandBuffer[imageIndex], 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(GbufferCommandBuffer[imageIndex], indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		renderObjectsIndexPass(objectsToRenderForFrame, imageIndex, GbufferCommandBuffer, static_cast<uint32_t>(indices.size()), objectHash);
		renderDrawGround(imageIndex, GbufferCommandBuffer, static_cast<uint32_t>(indices_ground.size()), vertexBuffer_ground, indexBuffer_ground);
		renderDrawAvatar(imageIndex, GbufferCommandBuffer, static_cast<uint32_t>(indices_avatar.size()), vertexBuffer_avatar, indexBuffer_avatar);
		
		vkCmdNextSubpass(GbufferCommandBuffer[imageIndex], VK_SUBPASS_CONTENTS_INLINE);

		//depth info
		vkCmdBindPipeline(GbufferCommandBuffer[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, GBufferPipelines.depthInfo);

		vkCmdBindDescriptorSets(GbufferCommandBuffer[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, GBufferPipelineLayout, 0, 1, &descriptorSetGBuffer[imageIndex], 0, nullptr);
		vkCmdBindVertexBuffers(GbufferCommandBuffer[imageIndex], 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(GbufferCommandBuffer[imageIndex], indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		
		renderObjectsIndexPass(objectsToRenderForFrame, imageIndex, GbufferCommandBuffer, static_cast<uint32_t>(indices.size()), objectHash);
		renderDrawGround(imageIndex, GbufferCommandBuffer, static_cast<uint32_t>(indices_ground.size()), vertexBuffer_ground, indexBuffer_ground);
		renderDrawAvatar(imageIndex, GbufferCommandBuffer, static_cast<uint32_t>(indices_avatar.size()), vertexBuffer_avatar, indexBuffer_avatar);
		
		vkCmdNextSubpass(GbufferCommandBuffer[imageIndex], VK_SUBPASS_CONTENTS_INLINE);

		//world coords
		vkCmdBindPipeline(GbufferCommandBuffer[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, GBufferPipelines.worldCoord);

		vkCmdBindDescriptorSets(GbufferCommandBuffer[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, GBufferPipelineLayout, 0, 1, &descriptorSetGBuffer[imageIndex], 0, nullptr);
		vkCmdBindVertexBuffers(GbufferCommandBuffer[imageIndex], 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(GbufferCommandBuffer[imageIndex], indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		
		renderObjectsIndexPass(objectsToRenderForFrame, imageIndex, GbufferCommandBuffer, static_cast<uint32_t>(indices.size()), objectHash);
		renderDrawGround(imageIndex, GbufferCommandBuffer, static_cast<uint32_t>(indices_ground.size()), vertexBuffer_ground, indexBuffer_ground);
		renderDrawAvatar(imageIndex, GbufferCommandBuffer, static_cast<uint32_t>(indices_avatar.size()), vertexBuffer_avatar, indexBuffer_avatar);
		
		// End the picking render pass
		vkCmdEndRenderPass(GbufferCommandBuffer[imageIndex]);

		// Finish recording the command buffer
		vkEndCommandBuffer(GbufferCommandBuffer[imageIndex]);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		// If you need to wait on any semaphores before starting the G-buffer pass
		VkSemaphore waitSemaphores{};
		VkPipelineStageFlags waitStages{ };
		submitInfo.waitSemaphoreCount = 0; // Update this count appropriately
		submitInfo.pWaitSemaphores = &waitSemaphores;
		submitInfo.pWaitDstStageMask = &waitStages;

		VkSemaphore signalSemaphores[] = { gBufferCompleteSemaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		// Setting the command buffer for the G-buffer pass
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &GbufferCommandBuffer[imageIndex];

		vkResetFences(device, 1, &GBufferFence[currentFrame]);

		check_vk_result(vkQueueSubmit(graphicsQueue, 1, &submitInfo, GBufferFence[currentFrame]));
	}

	void MainVulkApplication::GbufferRenderPipelineSetup() {

		FrustCullThreadPool = new Thread_pool_frustrum_culling();
		gbufferImageViews.resize(swapChainImages.size());

		for (size_t i = 0; i < swapChainImages.size(); ++i) {
			createImage(swapChainExtent.width, swapChainExtent.height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				gbufferImageViews[i].AlbedoImage, gbufferImageViews[i].AlbedoImageMemory);
			gbufferImageViews[i].AlbedoImageView = createImageView(gbufferImageViews[i].AlbedoImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

			createImage(swapChainExtent.width, swapChainExtent.height, VK_FORMAT_R32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				gbufferImageViews[i].DepthInfoImage, gbufferImageViews[i].DepthInfoImageMemory);
			gbufferImageViews[i].DepthInfoImageView = createImageView(gbufferImageViews[i].DepthInfoImage, VK_FORMAT_R32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);

			createImage(swapChainExtent.width, swapChainExtent.height, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				gbufferImageViews[i].NormalsImage, gbufferImageViews[i].NormalsImageMemory);
			gbufferImageViews[i].NormalsImageView = createImageView(gbufferImageViews[i].NormalsImage, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);

			createImage(swapChainExtent.width, swapChainExtent.height, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				gbufferImageViews[i].WorldCoordImage, gbufferImageViews[i].WorldCoordImageMemory);
			gbufferImageViews[i].WorldCoordImageView = createImageView(gbufferImageViews[i].WorldCoordImage, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);

		}

		//creating depth image, memory and view
		for (size_t i = 0; i < swapChainImages.size(); ++i) {
			VkImageCreateInfo depthImageInfo = {};
			depthImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
			depthImageInfo.extent = { swapChainExtent.width, swapChainExtent.height, 1 };
			depthImageInfo.mipLevels = 1;
			depthImageInfo.arrayLayers = 1;
			depthImageInfo.format = findDepthFormat(); // You'll need a function to select the appropriate depth format
			depthImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			depthImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT; // Depth stencil attachment
			depthImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			depthImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			if (vkCreateImage(device, &depthImageInfo, nullptr, &gbufferImageViews[i].GBufferImageDepth) != VK_SUCCESS)
				throw std::runtime_error("failed to create image!");

			VkMemoryRequirements memRequirements;
			vkGetImageMemoryRequirements(device, gbufferImageViews[i].GBufferImageDepth, &memRequirements);

			VkMemoryAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			if (vkAllocateMemory(device, &allocInfo, nullptr, &gbufferImageViews[i].GBufferImageDepthMemory) != VK_SUCCESS)
				throw std::runtime_error("failed to allocate image memory!");

			vkBindImageMemory(device, gbufferImageViews[i].GBufferImageDepth, gbufferImageViews[i].GBufferImageDepthMemory, 0);
			gbufferImageViews[i].GBufferImageDepthView = createImageView(gbufferImageViews[i].GBufferImageDepth, findDepthFormat(), VK_IMAGE_ASPECT_DEPTH_BIT);

		}
		
		//creating descriptor set layout
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.pImmutableSamplers = nullptr;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutBinding fragmentLayoutBinding{};
		fragmentLayoutBinding.binding = 1;
		fragmentLayoutBinding.descriptorCount = 1;
		fragmentLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		fragmentLayoutBinding.pImmutableSamplers = nullptr;
		fragmentLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, fragmentLayoutBinding };
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayoutGBuffer) != VK_SUCCESS)
			throw std::runtime_error("failed to create descriptor set layout!");

		//creating subpass dependencies and attachments
		VkAttachmentDescription albedoAttachment{};
		albedoAttachment.format = VK_FORMAT_R8G8B8A8_UNORM;
		albedoAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		albedoAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		albedoAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		albedoAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		albedoAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkAttachmentDescription normalAttachment{};
		normalAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
		normalAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		normalAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		normalAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		normalAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		normalAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkAttachmentDescription depthInfoAttachment{};
		depthInfoAttachment.format = VK_FORMAT_R32_SFLOAT;
		depthInfoAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthInfoAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthInfoAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthInfoAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthInfoAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkAttachmentDescription worldCoordAttachment{};
		worldCoordAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
		worldCoordAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		worldCoordAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		worldCoordAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		worldCoordAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		worldCoordAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = findDepthFormat();
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentRefAlbedo{};
		colorAttachmentRefAlbedo.attachment = 0;
		colorAttachmentRefAlbedo.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentRefNormals{};
		colorAttachmentRefNormals.attachment = 1;
		colorAttachmentRefNormals.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentRefDepthInfo{};
		colorAttachmentRefDepthInfo.attachment = 2;
		colorAttachmentRefDepthInfo.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentRefWorldCoord{};
		colorAttachmentRefWorldCoord.attachment = 3;
		colorAttachmentRefWorldCoord.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 4;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpassAlbedo{};
		subpassAlbedo.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassAlbedo.colorAttachmentCount = 1;
		subpassAlbedo.pColorAttachments = &colorAttachmentRefAlbedo;
		subpassAlbedo.pDepthStencilAttachment = &depthAttachmentRef;

		VkSubpassDescription subpassNormals{};
		subpassNormals.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassNormals.colorAttachmentCount = 1;
		subpassNormals.pColorAttachments = &colorAttachmentRefNormals;
		subpassNormals.pDepthStencilAttachment = &depthAttachmentRef;

		VkSubpassDescription subpassDepthInfo{};
		subpassDepthInfo.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDepthInfo.colorAttachmentCount = 1;
		subpassDepthInfo.pColorAttachments = &colorAttachmentRefDepthInfo;
		subpassDepthInfo.pDepthStencilAttachment = &depthAttachmentRef;

		VkSubpassDescription subpassWorldCoord{};
		subpassWorldCoord.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassWorldCoord.colorAttachmentCount = 1;
		subpassWorldCoord.pColorAttachments = &colorAttachmentRefWorldCoord;
		subpassWorldCoord.pDepthStencilAttachment = &depthAttachmentRef;

		VkSubpassDependency dependency1{};
		dependency1.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency1.dstSubpass = 0;
		dependency1.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency1.srcAccessMask = 0;
		dependency1.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency1.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		VkSubpassDependency dependency2{};
		dependency2.srcSubpass = 0;
		dependency2.dstSubpass = 1;
		dependency2.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency2.srcAccessMask = 0;
		dependency2.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency2.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		VkSubpassDependency dependency3{};
		dependency3.srcSubpass = 1;
		dependency3.dstSubpass = 2;
		dependency3.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency3.srcAccessMask = 0;
		dependency3.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency3.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		VkSubpassDependency dependency4{};
		dependency4.srcSubpass = 2;
		dependency4.dstSubpass = 3;
		dependency4.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency4.srcAccessMask = 0;
		dependency4.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency4.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		VkSubpassDependency dependencyToEnd{};
		dependencyToEnd.srcSubpass = 3;
		dependencyToEnd.dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencyToEnd.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencyToEnd.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencyToEnd.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencyToEnd.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		std::array<VkAttachmentDescription, 5> attachments = { albedoAttachment, normalAttachment, depthInfoAttachment,worldCoordAttachment, depthAttachment };
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		std::array<VkSubpassDescription, 4> subpasses = { subpassAlbedo, subpassNormals, subpassDepthInfo,subpassWorldCoord };
		renderPassInfo.subpassCount = subpasses.size();
		renderPassInfo.pSubpasses = subpasses.data();
		std::array<VkSubpassDependency, 5> dependencies = { dependency1, dependency2,dependency3,dependency4,dependencyToEnd };
		renderPassInfo.dependencyCount = dependencies.size();
		renderPassInfo.pDependencies = dependencies.data();

		if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPassGBuffer) != VK_SUCCESS) 
			throw std::runtime_error("failed to create render pass!");
		
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayoutGBuffer;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		check_vk_result(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &GBufferPipelineLayout));
		
		auto vertShaderCode = readFile("shaders/gbuffer_albedo_vert.spv");
		auto fragShaderCode = readFile("shaders/gbuffer_albedo_frag.spv");
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;
		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);
		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};

		//initial pipeline layout for albedo
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		shaderStages[0] = vertShaderStageInfo;
		shaderStages[1] = fragShaderStageInfo;

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

		// Vertex input bindings
		bindingDescriptions.push_back(Vertex::getBindingDescription());
		{
			auto tempVertAttrib = Vertex::getAttributeDescriptions();
			attributeDescriptions.insert(end(attributeDescriptions), begin(tempVertAttrib), end(tempVertAttrib));
		}

		vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapChainExtent.width;
		viewport.height = (float)swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = swapChainExtent;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		//depthStencil.depthWriteEnable = VK_FALSE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;
			
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = shaderStages.size();
		pipelineInfo.pStages = shaderStages.data();
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.layout = GBufferPipelineLayout;
		pipelineInfo.renderPass = renderPassGBuffer;
		pipelineInfo.subpass = 0;

		check_vk_result(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &GBufferPipelines.albedo));
		
		// pipeline layout for normals
		vertShaderCode = readFile("shaders/gbuffer_normals_vert.spv");
		fragShaderCode = readFile("shaders/gbuffer_normals_frag.spv");

		vertShaderModule = createShaderModule(vertShaderCode);
		fragShaderModule = createShaderModule(fragShaderCode);

		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		shaderStages[0] = vertShaderStageInfo;
		shaderStages[1] = fragShaderStageInfo;
		pipelineInfo.stageCount = shaderStages.size();
		pipelineInfo.pStages = shaderStages.data();
		pipelineInfo.subpass = 1;
			
		check_vk_result(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &GBufferPipelines.normals));
		
		//pipeline layout for depth info
		vertShaderCode = readFile("shaders/gbuffer_depthInfo_vert.spv");
		fragShaderCode = readFile("shaders/gbuffer_depthInfo_frag.spv");

		vertShaderModule = createShaderModule(vertShaderCode);
		fragShaderModule = createShaderModule(fragShaderCode);

		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		shaderStages[0] = vertShaderStageInfo;
		shaderStages[1] = fragShaderStageInfo;
		pipelineInfo.stageCount = shaderStages.size();
		pipelineInfo.pStages = shaderStages.data();
		pipelineInfo.subpass = 2;

		check_vk_result(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &GBufferPipelines.depthInfo));
		
		//pipeline layout for world coords
		vertShaderCode = readFile("shaders/gbuffer_worldCoord_vert.spv");
		fragShaderCode = readFile("shaders/gbuffer_worldCoord_frag.spv");

		vertShaderModule = createShaderModule(vertShaderCode);
		fragShaderModule = createShaderModule(fragShaderCode);

		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		shaderStages[0] = vertShaderStageInfo;
		shaderStages[1] = fragShaderStageInfo;
		pipelineInfo.stageCount = shaderStages.size();
		pipelineInfo.pStages = shaderStages.data();
		pipelineInfo.subpass = 3;

		check_vk_result(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &GBufferPipelines.worldCoord));

		vkDestroyShaderModule(device, fragShaderModule, nullptr);
		vkDestroyShaderModule(device, vertShaderModule, nullptr);

		GBufferFramebuffer.resize(swapChainFramebuffers.size());
		//creating framebuffers
		for (size_t i = 0; i < swapChainFramebuffers.size(); ++i) {
			std::array<VkImageView, 5> GBufferImageViewattachments = { 
				gbufferImageViews[i].AlbedoImageView, gbufferImageViews[i].NormalsImageView, gbufferImageViews[i].DepthInfoImageView,
				gbufferImageViews[i].WorldCoordImageView, gbufferImageViews[i].GBufferImageDepthView };

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPassGBuffer;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(GBufferImageViewattachments.size());
			framebufferInfo.pAttachments = GBufferImageViewattachments.data();
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			check_vk_result(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &GBufferFramebuffer[i]));
		}

		GBufferFence.resize(swapChainImages.size());
		for (size_t i = 0; i < swapChainImages.size(); ++i) {
			VkFenceCreateInfo fenceInfo{};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			check_vk_result(vkCreateFence(device, &fenceInfo, nullptr, &GBufferFence[i]));
		}
		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		check_vk_result(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &gBufferCompleteSemaphore));
	}

	void MainVulkApplication::createDescriptorSetGBuffer() {

		uint32_t swapChainImageCount = static_cast<uint32_t>(swapChainImages.size());

		// Each descriptor set will have 2 uniform buffer descriptors
		VkDescriptorPoolSize poolSizes[] = {
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 * swapChainImageCount }
		};

		VkDescriptorPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCreateInfo.poolSizeCount = sizeof(poolSizes) / sizeof(poolSizes[0]);
		poolCreateInfo.pPoolSizes = poolSizes;
		// Specify the maximum number of descriptor sets that can be allocated from the pool
		poolCreateInfo.maxSets = swapChainImageCount;
		
		if (vkCreateDescriptorPool(device, &poolCreateInfo, nullptr, &GBufferDescriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create G-buffer descriptor pool!");
		}

		std::vector<VkDescriptorSetLayout> layouts(swapChainImages.size(), descriptorSetLayoutGBuffer);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = GBufferDescriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());;
		allocInfo.pSetLayouts = layouts.data();

		descriptorSetGBuffer.resize(swapChainImages.size());

		if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSetGBuffer.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		for (size_t i = 0; i < swapChainImages.size(); i++) {

			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkDescriptorBufferInfo bufferFragInfo = {};
			bufferFragInfo.buffer = uniformFragBuffers[i];
			bufferFragInfo.offset = 0;
			bufferFragInfo.range = sizeof(UniformFragmentObject);

			std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = descriptorSetGBuffer[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = descriptorSetGBuffer[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pBufferInfo = &bufferFragInfo;

			vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}

	void MainVulkApplication::setupGBufferCommandBuffer() {
		GbufferCommandBuffer.resize(swapChainFramebuffers.size());

		// Create a command buffer for picking
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;  // Assuming you've created a command pool
		allocInfo.commandBufferCount = GbufferCommandBuffer.size();

		vkAllocateCommandBuffers(device, &allocInfo, GbufferCommandBuffer.data());
	}
}
#endif