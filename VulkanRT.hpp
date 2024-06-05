#ifndef __VK_RT_HPP__
#define __VK_RT_HPP__

namespace VkApplication {

	glm::vec3 sphereCenter ( 2.0f, 2.0f, 2.0f );
	float sphereRadius = 3.0f;
	// Define AABB for a procedural sphere
	VkAabbPositionsKHR aabbSphere({ sphereCenter.x - sphereRadius,
									sphereCenter.y - sphereRadius,
									sphereCenter.z - sphereRadius,
									sphereCenter.x + sphereRadius,
									sphereCenter.y + sphereRadius,
									sphereCenter.z + sphereRadius });

	void MainVulkApplication::DrawRT() {

		vkWaitForFences(device, 1, &renderFenceRT, VK_TRUE, UINT64_MAX);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores = {};
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR };
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = &waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBufferRT;

		VkSemaphore signalSemaphores[] = { finishedSemaphoreRT };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, renderFenceRT) != VK_SUCCESS) {
			throw std::runtime_error("Failed to submit draw command buffer");
		}
	}

	void MainVulkApplication::recordCommandBuffer() {

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		if (vkBeginCommandBuffer(commandBufferRT, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed to begin recording command buffer");
		}

		// Transition the image layout if necessary (depends on your specific case)
		// transitionImageLayouts(commandBuffer);

		// Bind ray tracing pipeline
		vkCmdBindPipeline(commandBufferRT, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rayTracingPipeline);

		// Bind descriptor sets
		vkCmdBindDescriptorSets(commandBufferRT, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineLayoutRT,
			0, 1, &descriptorSetRT, 0, nullptr);

		// Define the shader binding table regions
		VkStridedDeviceAddressRegionKHR raygenShaderBindingTable{};
		raygenShaderBindingTable.deviceAddress = shaderBindingTables.raygen.stridedDeviceAddressRegion.deviceAddress;
		raygenShaderBindingTable.stride = shaderBindingTables.raygen.stridedDeviceAddressRegion.stride;
		raygenShaderBindingTable.size = shaderBindingTables.raygen.stridedDeviceAddressRegion.size;

		VkStridedDeviceAddressRegionKHR missShaderBindingTable{};
		missShaderBindingTable.deviceAddress = shaderBindingTables.miss.stridedDeviceAddressRegion.deviceAddress;
		missShaderBindingTable.stride = shaderBindingTables.miss.stridedDeviceAddressRegion.stride;
		missShaderBindingTable.size = shaderBindingTables.miss.stridedDeviceAddressRegion.size;

		VkStridedDeviceAddressRegionKHR hitShaderBindingTable{};
		hitShaderBindingTable.deviceAddress = shaderBindingTables.hit.stridedDeviceAddressRegion.deviceAddress;
		hitShaderBindingTable.stride = shaderBindingTables.hit.stridedDeviceAddressRegion.stride;
		hitShaderBindingTable.size = shaderBindingTables.hit.stridedDeviceAddressRegion.size;

		VkStridedDeviceAddressRegionKHR callableShaderBindingTable{};

		// Issue the ray tracing command
		vkCmdTraceRaysKHR(commandBufferRT, &raygenShaderBindingTable, &missShaderBindingTable,
			&hitShaderBindingTable, &callableShaderBindingTable, swapChainExtent.width, swapChainExtent.height, 1);

		// Transition the image layout if necessary (depends on your specific case)
		// transitionImageLayoutsForSampling(commandBuffer);

		if (vkEndCommandBuffer(commandBufferRT) != VK_SUCCESS) {
			throw std::runtime_error("Failed to record command buffer");
		}
	}

	void MainVulkApplication::setupAS() {

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, aabbBuffer_, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		VkDeviceOrHostAddressConstKHR aabbBufferDeviceAddress{};

		VkBufferDeviceAddressInfoKHR bufferDeviceAI{};
		bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		bufferDeviceAI.buffer = aabbBuffer_;
		aabbBufferDeviceAddress.deviceAddress = vkGetBufferDeviceAddressKHR(device, &bufferDeviceAI);

		VkAccelerationStructureGeometryKHR geometry = {};
		geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		geometry.pNext = nullptr;
		geometry.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
		geometry.geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
		geometry.geometry.aabbs.pNext = nullptr;
		geometry.geometry.aabbs.data = aabbBufferDeviceAddress;
		//geometry.geometry.aabbs.data.deviceAddress = aabbBufferMemory_;
		geometry.geometry.aabbs.stride = sizeof(VkAabbPositionsKHR);
		geometry.flags =  VK_GEOMETRY_OPAQUE_BIT_KHR;

		VkAccelerationStructureBuildGeometryInfoKHR blasBuildInfo;
		blasBuildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		blasBuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		blasBuildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		blasBuildInfo.geometryCount = 1;
		blasBuildInfo.pGeometries = &geometry;

		uint32_t primitive_count = 1;

		VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfoKHR{};
		accelerationStructureBuildSizesInfoKHR.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
		vkGetAccelerationStructureBuildSizesKHR( device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
												 &blasBuildInfo, &primitive_count, &accelerationStructureBuildSizesInfoKHR);
		
		// Create buffer for the acceleration structure
		VkBufferCreateInfo asBufferInfo{};
		asBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		asBufferInfo.size = accelerationStructureBuildSizesInfoKHR.accelerationStructureSize;
		asBufferInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		asBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &asBufferInfo, nullptr, &asBuffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create AS buffer");
		}

		vkGetBufferMemoryRequirements(device, asBuffer, &memRequirements);

		VkMemoryAllocateInfo asAllocInfo{};
		asAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		asAllocInfo.allocationSize = memRequirements.size;
		asAllocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		VkDeviceMemory asMemory;
		if (vkAllocateMemory(device, &asAllocInfo, nullptr, &asMemory) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate AS memory");
		}

		vkBindBufferMemory(device, asBuffer, asMemory, 0);

		VkAccelerationStructureCreateInfoKHR asCreateInfo{};
		asCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		asCreateInfo.buffer = asBuffer;
		asCreateInfo.size = accelerationStructureBuildSizesInfoKHR.accelerationStructureSize;
		asCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

		VkAccelerationStructureKHR blas;
		if (vkCreateAccelerationStructureKHR(device, &asCreateInfo, nullptr, &blas) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create BLAS");
		}

		// Step 3: Build acceleration structure
		VkBufferCreateInfo scratchBufferInfo{};
		scratchBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		scratchBufferInfo.size = accelerationStructureBuildSizesInfoKHR.buildScratchSize;
		scratchBufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		scratchBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkBuffer scratchBuffer;
		if (vkCreateBuffer(device, &scratchBufferInfo, nullptr, &scratchBuffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create scratch buffer");
		}

		vkGetBufferMemoryRequirements(device, scratchBuffer, &memRequirements);

		VkMemoryAllocateInfo scratchAllocInfo{};
		scratchAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		scratchAllocInfo.allocationSize = memRequirements.size;
		scratchAllocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		VkDeviceMemory scratchMemory;
		if (vkAllocateMemory(device, &scratchAllocInfo, nullptr, &scratchMemory) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate scratch memory");
		}

		vkBindBufferMemory(device, scratchBuffer, scratchMemory, 0);

		VkBufferDeviceAddressInfoKHR scratchBufferDeviceAI{};
		scratchBufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		scratchBufferDeviceAI.buffer = scratchBuffer;
		VkDeviceAddress scratchBufferDeviceAddress = vkGetBufferDeviceAddressKHR(device, &scratchBufferDeviceAI);

		blasBuildInfo.scratchData.deviceAddress = scratchBufferDeviceAddress;

		VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
		buildRangeInfo.primitiveCount = primitive_count;
		buildRangeInfo.primitiveOffset = 0;
		buildRangeInfo.firstVertex = 0;
		buildRangeInfo.transformOffset = 0;

		const VkAccelerationStructureBuildRangeInfoKHR* pBuildRangeInfo = &buildRangeInfo;

		VkCommandBuffer commandBuffer = beginSingleTimeCommands(); // Assume you have this function
		vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &blasBuildInfo, &pBuildRangeInfo);
		endSingleTimeCommands(commandBuffer); // Assume you have this function

		// Store the created BLAS
		blas_ = blas;
	}

	void MainVulkApplication::descriptorLayoutSetupRT() {

		VkDescriptorSetLayoutBinding uniformLayoutBinding{};
		uniformLayoutBinding.binding = 0;
		uniformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformLayoutBinding.descriptorCount = 1;
		uniformLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

		VkDescriptorSetLayoutBinding colorImageLayoutBinding{};
		colorImageLayoutBinding.binding = 1;
		colorImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		colorImageLayoutBinding.descriptorCount = 1;
		colorImageLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		colorImageLayoutBinding.pImmutableSamplers = nullptr; // Not needed for image

		VkDescriptorSetLayoutBinding depthImageLayoutBinding{};
		depthImageLayoutBinding.binding = 2;
		depthImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		depthImageLayoutBinding.descriptorCount = 1;
		depthImageLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		depthImageLayoutBinding.pImmutableSamplers = nullptr; // Not needed for image

		std::array<VkDescriptorSetLayoutBinding, 3> bindings = { uniformLayoutBinding, colorImageLayoutBinding, depthImageLayoutBinding };
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayoutRT) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor set layout!");
		}

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool; // Assume you have created a descriptor pool
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &descriptorSetLayoutRT;

		if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSetRT) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor set!");
		}

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;  // Use more if you have multiple descriptor set layouts
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayoutRT;
		pipelineLayoutInfo.pushConstantRangeCount = 0;  // Add push constant ranges if needed

		check_vk_result(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayoutRT));

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = uniformBuffers[0]; // Assuming you've created a uniform buffer
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject); // Size of your uniform buffer structure

		VkDescriptorImageInfo colorImageInfo{};
		colorImageInfo.imageView = rtImageViews.RTColorImageView; // Assuming you've created an image view
		colorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		
		VkDescriptorImageInfo depthImageInfo{};
		depthImageInfo.imageView = rtImageViews.RTDepthImageView; // Assuming you've created an image view
		depthImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSetRT;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = descriptorSetRT;
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &colorImageInfo;

		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = descriptorSetRT;
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pImageInfo = &depthImageInfo;

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}

	void MainVulkApplication::setupRT() {

		// Get ray tracing pipeline properties, which will be used later on in the sample
		rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
		VkPhysicalDeviceProperties2 deviceProperties2{};
		deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		deviceProperties2.pNext = &rayTracingPipelineProperties;
		vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperties2);

		// Get acceleration structure properties, which will be used later on in the sample
		accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
		VkPhysicalDeviceFeatures2 deviceFeatures2{};
		deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		deviceFeatures2.pNext = &accelerationStructureFeatures;
		vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

		createAABBBuffer();
		setupAS();

		createImage(swapChainExtent.width, swapChainExtent.height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			rtImageViews.RTColorImage, rtImageViews.RTColorImageMemory);
		rtImageViews.RTColorImageView = createImageView(rtImageViews.RTColorImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

		createImage(swapChainExtent.width, swapChainExtent.height, VK_FORMAT_R32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			rtImageViews.RTDepthImage, rtImageViews.RTDepthImageMemory);
		rtImageViews.RTDepthImageView = createImageView(rtImageViews.RTDepthImage, VK_FORMAT_R32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);

		auto rayGShaderCode = readFile("shaders/RT_raygen.spv");
		auto missRTShaderCode = readFile("shaders/RT_miss.spv");
		auto closeHitRTShaderCode = readFile("shaders/RT_closesthit.spv");
		auto intersectRTShaderCode = readFile("shaders/RT_intersection.spv");

		VkShaderModule rayGShaderModule = createShaderModule(rayGShaderCode);
		VkShaderModule missRTShaderModule = createShaderModule(missRTShaderCode);
		VkShaderModule closeHitRTShaderModule = createShaderModule(closeHitRTShaderCode);
		VkShaderModule intersectRTShaderModule = createShaderModule(intersectRTShaderCode);

		VkPipelineShaderStageCreateInfo raygenShaderStageInfo{};
		raygenShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		raygenShaderStageInfo.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		raygenShaderStageInfo.module = rayGShaderModule;
		raygenShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo missRTShaderStageInfo{};
		missRTShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		missRTShaderStageInfo.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
		missRTShaderStageInfo.module = missRTShaderModule;
		missRTShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo closeHitRTShaderStageInfo{};
		closeHitRTShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		closeHitRTShaderStageInfo.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		closeHitRTShaderStageInfo.module = closeHitRTShaderModule;
		closeHitRTShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo intersectRTShaderStageInfo{};
		intersectRTShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		intersectRTShaderStageInfo.stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
		intersectRTShaderStageInfo.module = intersectRTShaderModule;
		intersectRTShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { raygenShaderStageInfo, missRTShaderStageInfo, closeHitRTShaderStageInfo,intersectRTShaderStageInfo };

		VkRayTracingShaderGroupCreateInfoKHR raygenGroup{};
		raygenGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		raygenGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		raygenGroup.generalShader = 0;  // Index of raygen shader in pStages
		raygenGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
		raygenGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		raygenGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		shaderGroups.push_back(raygenGroup);

		VkRayTracingShaderGroupCreateInfoKHR missGroup{};
		missGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		missGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;  // General type for miss shaders
		missGroup.generalShader = 1;  // Index in pStages array
		missGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
		missGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		missGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		shaderGroups.push_back(missGroup);

		VkRayTracingShaderGroupCreateInfoKHR hitGroup{};
		hitGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		hitGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;  // For triangle geometry
		hitGroup.generalShader = VK_SHADER_UNUSED_KHR;  // Not used in hit groups
		hitGroup.closestHitShader = 2;  // Index in pStages array
		hitGroup.anyHitShader = VK_SHADER_UNUSED_KHR;  // Optional: VK_SHADER_UNUSED_KHR if not used
		hitGroup.intersectionShader = 3;  // Not used for triangles
		shaderGroups.push_back(hitGroup);

		VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCI{};
		rayTracingPipelineCI.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		rayTracingPipelineCI.pNext = nullptr;
		rayTracingPipelineCI.flags = 0;
		rayTracingPipelineCI.stageCount = sizeof(shaderStages) / sizeof(shaderStages[0]); // Number of shader stages
		rayTracingPipelineCI.pStages = shaderStages;  // Pointer to shader stages array
		rayTracingPipelineCI.groupCount = 3;  // Number of shader groups
		VkRayTracingShaderGroupCreateInfoKHR shaderGroups[] = { raygenGroup, missGroup, hitGroup };
		rayTracingPipelineCI.pGroups = shaderGroups;  // Pointer to shader groups array
		rayTracingPipelineCI.maxPipelineRayRecursionDepth = 1;  // Adjust based on your ray tracing depth requirements
		rayTracingPipelineCI.layout = pipelineLayoutRT;  // The pipeline layout that matches the descriptors used by the shaders
		rayTracingPipelineCI.basePipelineHandle = VK_NULL_HANDLE;  // Not deriving from an existing pipeline
		rayTracingPipelineCI.basePipelineIndex = -1;  // Not deriving from an existing pipeline

		// Declare a pointer to the function
		PFN_vkCreateRayTracingPipelinesKHR pfnVkCreateRayTracingPipelinesKHR = nullptr;

		// Load the function after creating the device
		pfnVkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));
		if (!pfnVkCreateRayTracingPipelinesKHR) {
			throw std::runtime_error("Could not load vkCreateRayTracingPipelinesKHR");
		}
		
		VkResult result = pfnVkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayTracingPipelineCI, nullptr, &rayTracingPipeline);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to create ray tracing pipeline");
		}
		
		//check_vk_result(vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE,1, &rayTracingPipelineCI, nullptr, &rayTracingPipeline));

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Create the fence in a signaled state

		if (vkCreateFence(device, &fenceInfo, nullptr, &renderFenceRT) != VK_SUCCESS) {
			throw std::runtime_error("failed to create synchronization fence!");
		}

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &finishedSemaphoreRT) != VK_SUCCESS) {
			throw std::runtime_error("failed to create semaphores!");
		}

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		check_vk_result(vkAllocateCommandBuffers(device, &allocInfo, &commandBufferRT));
	}

	void MainVulkApplication::createShaderBindingTable(ShaderBindingTable& shaderBindingTable, uint32_t handleCount) {

		createBuffer(rayTracingPipelineProperties.shaderGroupHandleSize * handleCount,
			VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			shaderBindingTable.buffer, shaderBindingTable.memory);
		// Get the strided address to be used when dispatching the rays

		const uint32_t handleSizeAligned = align_up(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);
		VkStridedDeviceAddressRegionKHR stridedDeviceAddressRegionKHR{};

		VkBufferDeviceAddressInfoKHR bufferDeviceAI{};
		bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		bufferDeviceAI.buffer = shaderBindingTable.buffer;
		vkGetBufferDeviceAddressKHR(device, &bufferDeviceAI);

		stridedDeviceAddressRegionKHR.deviceAddress = vkGetBufferDeviceAddressKHR(device, &bufferDeviceAI);
		stridedDeviceAddressRegionKHR.stride = handleSizeAligned;
		stridedDeviceAddressRegionKHR.size = handleCount * handleSizeAligned;

		shaderBindingTable.stridedDeviceAddressRegion = stridedDeviceAddressRegionKHR;
		// Map persistent 
		shaderBindingTable.map();
	}

	/*
		Create the Shader Binding Tables that binds the programs and top-level acceleration structure

		SBT Layout used in this sample:

			/-----------\
			| raygen    |
			|-----------|
			| miss      |
			|-----------|
			| hit + int |
			\-----------/

	*/
	void MainVulkApplication::createSBT() {
		const uint32_t handleSize = rayTracingPipelineProperties.shaderGroupHandleSize;
		const uint32_t handleSizeAligned = align_up(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);
		const uint32_t groupCount = static_cast<uint32_t>(shaderGroups.size());
		const uint32_t sbtSize = groupCount * handleSizeAligned;

		std::vector<uint8_t> shaderHandleStorage(sbtSize);
		check_vk_result(vkGetRayTracingShaderGroupHandlesKHR(device, rayTracingPipeline, 0, groupCount, sbtSize, shaderHandleStorage.data()));

		createShaderBindingTable(shaderBindingTables.raygen, 1);
		createShaderBindingTable(shaderBindingTables.miss, 1);
		createShaderBindingTable(shaderBindingTables.hit, 1);

		// Copy handles
		memcpy(shaderBindingTables.raygen.mapped, shaderHandleStorage.data(), handleSize);
		memcpy(shaderBindingTables.miss.mapped, shaderHandleStorage.data() + handleSizeAligned, handleSize);
		memcpy(shaderBindingTables.hit.mapped, shaderHandleStorage.data() + handleSizeAligned * 2, handleSize);
	}

	void MainVulkApplication::createAABBBuffer() {

		VkDeviceSize bufferSize = sizeof(VkAabbPositionsKHR);

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, &aabbSphere, static_cast<size_t>(bufferSize));
		vkUnmapMemory(device, stagingBufferMemory);

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, aabbBuffer_, aabbBufferMemory_);

		copyBuffer(stagingBuffer, aabbBuffer_, bufferSize);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}
}
#endif
