#ifndef __VK_RT_HPP__
#define __VK_RT_HPP__

namespace VkApplication {

	glm::vec3 sphereCenter ( 2.0f, 2.0f, 2.0f );
	float sphereRadius = 1.0f;
	// Define AABB for a procedural sphere
	VkAabbPositionsKHR aabbSphere({ sphereCenter.x - sphereRadius,
									sphereCenter.y - sphereRadius,
									sphereCenter.z - sphereRadius,
									sphereCenter.x + sphereRadius,
									sphereCenter.y + sphereRadius,
									sphereCenter.z + sphereRadius });

	void MainVulkApplication::DrawRT() {

		vkWaitForFences(device, 1, &renderFenceRT, VK_TRUE, UINT64_MAX);

		PushConstants pushConstants;
		pushConstants.avatarPos = glm::vec3(0.0, 0.5, 0.0);
		pushConstants.timeStamp = static_cast<float>(secondCount);
		/*
		vkCmdPushConstants(commandBufferRT, pipelineLayoutRT, 
			VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 0, sizeof(PushConstants), &pushConstants);
		*/

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		
		check_vk_result(vkResetCommandBuffer(commandBufferRT, 0));

		if (vkBeginCommandBuffer(commandBufferRT, &beginInfo) != VK_SUCCESS) 
			throw std::runtime_error("Failed to begin recording command buffer");

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

		vkCmdPushConstants(commandBufferRT, pipelineLayoutRT,
			VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 0, sizeof(PushConstants), &pushConstants);
		// Issue the ray tracing command
		vkCmdTraceRaysKHR(commandBufferRT, &raygenShaderBindingTable, &missShaderBindingTable,
			&hitShaderBindingTable, &callableShaderBindingTable, swapChainExtent.width, swapChainExtent.height, 1);

		// Transition the image layout if necessary (depends on your specific case)
		// transitionImageLayoutsForSampling(commandBuffer);

		if (vkEndCommandBuffer(commandBufferRT) != VK_SUCCESS) 
			throw std::runtime_error("Failed to record command buffer");
		
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

		check_vk_result(vkResetFences(device, 1, &renderFenceRT));

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

		VkBufferDeviceAddressInfoKHR bufferDeviceAI{};
		bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		bufferDeviceAI.buffer = aabbBuffer_;
		VkDeviceAddress bufferDeviceAddressAABB = vkGetBufferDeviceAddressKHR(device, &bufferDeviceAI);

		// Build
		VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
		accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		// Instead of providing actual geometry (e.g. triangles), we only provide the axis aligned bounding boxes (AABBs) of the spheres
		// The data for the actual spheres is passed elsewhere as a shader storage buffer object
		accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
		accelerationStructureGeometry.geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
		accelerationStructureGeometry.geometry.aabbs.data.deviceAddress = bufferDeviceAddressAABB;
		accelerationStructureGeometry.geometry.aabbs.stride = sizeof(VkAabbPositionsKHR);

		// Get size info
		VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
		accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		accelerationStructureBuildGeometryInfo.geometryCount = 1;
		accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

		uint32_t aabbCount = 1;
		VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
		accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	
		vkGetAccelerationStructureBuildSizesKHR(
			device,
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&accelerationStructureBuildGeometryInfo,
			&aabbCount,
			&accelerationStructureBuildSizesInfo);

		createAccelerationStructure(bottomLevelAS, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, accelerationStructureBuildSizesInfo);

		// Create a small scratch buffer used during build of the bottom level acceleration structure
		createScratchBuffer(accelerationStructureBuildSizesInfo.buildScratchSize);

		VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
		accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		accelerationBuildGeometryInfo.dstAccelerationStructure = bottomLevelAS.handle;
		accelerationBuildGeometryInfo.geometryCount = 1;
		accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
		accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

		VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
		accelerationStructureBuildRangeInfo.primitiveCount = aabbCount;
		std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		// Build the acceleration structure on the device via a one-time command buffer submission
		// Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
		vkCmdBuildAccelerationStructuresKHR(
			commandBuffer,
			1,
			&accelerationBuildGeometryInfo,
			accelerationBuildStructureRangeInfos.data());

		endSingleTimeCommands(commandBuffer);

		if (scratchBuffer.memory != VK_NULL_HANDLE) 
			vkFreeMemory(device, scratchBuffer.memory, nullptr);
		if (scratchBuffer.handle != VK_NULL_HANDLE)
			vkDestroyBuffer(device, scratchBuffer.handle, nullptr);

		VkTransformMatrixKHR transformMatrix = {
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f };

		VkAccelerationStructureInstanceKHR instance{};
		instance.transform = transformMatrix;
		instance.instanceCustomIndex = 0;
		instance.mask = 0xFF;
		instance.instanceShaderBindingTableRecordOffset = 0;
		instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
		instance.accelerationStructureReference = bottomLevelAS.deviceAddress;

		// Buffer for instance data
		VkBuffer instancesBuffer; VkDeviceMemory instancesBufferMemory;
		createBuffer(sizeof(VkAccelerationStructureInstanceKHR),
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			instancesBuffer, instancesBufferMemory, true);

		void* mappedInstance = nullptr;

		check_vk_result(vkMapMemory(device, instancesBufferMemory, 0,
			sizeof(VkAccelerationStructureInstanceKHR), 0, &mappedInstance));
		memcpy(mappedInstance, &instance, sizeof(VkAccelerationStructureInstanceKHR));
		vkUnmapMemory(device, instancesBufferMemory);
		mappedInstance = NULL;

		VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
		VkBufferDeviceAddressInfoKHR bufferDeviceAInstance{};
		bufferDeviceAInstance.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		bufferDeviceAInstance.buffer = instancesBuffer;
		instanceDataDeviceAddress.deviceAddress = vkGetBufferDeviceAddressKHR(device, &bufferDeviceAInstance);

		VkAccelerationStructureGeometryKHR accelerationStructureGeometryI{};
		accelerationStructureGeometryI.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		accelerationStructureGeometryI.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		accelerationStructureGeometryI.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		accelerationStructureGeometryI.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		accelerationStructureGeometryI.geometry.instances.arrayOfPointers = VK_FALSE;
		accelerationStructureGeometryI.geometry.instances.data = instanceDataDeviceAddress;

		// Get size info
		VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfoI{};
		accelerationStructureBuildGeometryInfoI.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		accelerationStructureBuildGeometryInfoI.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		accelerationStructureBuildGeometryInfoI.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		accelerationStructureBuildGeometryInfoI.geometryCount = 1;
		accelerationStructureBuildGeometryInfoI.pGeometries = &accelerationStructureGeometryI;

		uint32_t primitive_count = 1;

		VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfoI{};
		accelerationStructureBuildSizesInfoI.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
		vkGetAccelerationStructureBuildSizesKHR(
			device,
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&accelerationStructureBuildGeometryInfoI,
			&primitive_count,
			&accelerationStructureBuildSizesInfoI);

		createAccelerationStructure(topLevelAS, VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, accelerationStructureBuildSizesInfoI);

		// Create a small scratch buffer used during build of the top level acceleration structure
		createScratchBuffer(accelerationStructureBuildSizesInfoI.buildScratchSize);

		VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfoTop{};
		accelerationBuildGeometryInfoTop.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		accelerationBuildGeometryInfoTop.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		accelerationBuildGeometryInfoTop.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		accelerationBuildGeometryInfoTop.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		accelerationBuildGeometryInfoTop.dstAccelerationStructure = topLevelAS.handle;
		accelerationBuildGeometryInfoTop.geometryCount = 1;
		accelerationBuildGeometryInfoTop.pGeometries = &accelerationStructureGeometryI;
		accelerationBuildGeometryInfoTop.scratchData.deviceAddress = scratchBuffer.deviceAddress;

		VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfoTop{};
		accelerationStructureBuildRangeInfoTop.primitiveCount = 1;
		accelerationStructureBuildRangeInfoTop.primitiveOffset = 0;
		accelerationStructureBuildRangeInfoTop.firstVertex = 0;
		accelerationStructureBuildRangeInfoTop.transformOffset = 0;

		accelerationBuildStructureRangeInfos.clear();
		accelerationBuildStructureRangeInfos.push_back(&accelerationStructureBuildRangeInfoTop);

		VkCommandBuffer commandBuffer1 = beginSingleTimeCommands();
		// Build the acceleration structure on the device via a one-time command buffer submission
		// Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
		
		vkCmdBuildAccelerationStructuresKHR(
			commandBuffer1,
			1,
			&accelerationBuildGeometryInfoTop,
			accelerationBuildStructureRangeInfos.data());
		endSingleTimeCommands(commandBuffer1);

		if (scratchBuffer.memory != VK_NULL_HANDLE) 
			vkFreeMemory(device, scratchBuffer.memory, nullptr);
		if (scratchBuffer.handle != VK_NULL_HANDLE) 
			vkDestroyBuffer(device, scratchBuffer.handle, nullptr);
		if (instancesBuffer)
			vkDestroyBuffer(device, instancesBuffer, nullptr);
		if (instancesBufferMemory)
			vkFreeMemory(device, instancesBufferMemory, nullptr);

	}

	void MainVulkApplication::descriptorLayoutSetupRT() {

		VkDescriptorSetLayoutBinding accelLayoutBinding{};
		accelLayoutBinding.binding = 0;
		accelLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		accelLayoutBinding.descriptorCount = 1;
		accelLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR ;

		VkDescriptorSetLayoutBinding uniformLayoutBinding{};
		uniformLayoutBinding.binding = 1;
		uniformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformLayoutBinding.descriptorCount = 1;
		uniformLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR ;

		VkDescriptorSetLayoutBinding colorImageLayoutBinding{};
		colorImageLayoutBinding.binding = 2;
		colorImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		colorImageLayoutBinding.descriptorCount = 1;
		colorImageLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		colorImageLayoutBinding.pImmutableSamplers = nullptr; // Not needed for image

		VkDescriptorSetLayoutBinding depthImageLayoutBinding{};
		depthImageLayoutBinding.binding = 3;
		depthImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		depthImageLayoutBinding.descriptorCount = 1;
		depthImageLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		depthImageLayoutBinding.pImmutableSamplers = nullptr; // Not needed for image

		std::array<VkDescriptorSetLayoutBinding, 4> bindings = { accelLayoutBinding, uniformLayoutBinding, colorImageLayoutBinding, depthImageLayoutBinding };
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayoutRT) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	void MainVulkApplication::createDescriptorSetsRT() {

		std::vector<VkDescriptorPoolSize> poolSizes = {
			{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 }
		};

		VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
		descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();
		descriptorPoolCreateInfo.maxSets = 1;
		check_vk_result(vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &descriptorPoolRT));

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPoolRT; // Assume you have created a descriptor pool
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &descriptorSetLayoutRT;

		if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSetRT) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor set!");
		}

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(PushConstants); // Replace PushConstantData with your actual structure

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;  // Use more if you have multiple descriptor set layouts
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayoutRT;
		pipelineLayoutInfo.pushConstantRangeCount = 1;  // Add push constant ranges if needed
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		check_vk_result(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayoutRT));

		VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo{};
		descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
		descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
		descriptorAccelerationStructureInfo.pAccelerationStructures = &topLevelAS.handle;

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = uniformBuffers[0]; // Assuming you've created a uniform buffer
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject); // Size of your uniform buffer structure

		//transitionImageLayout(rtImageViews.RTColorImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		//transitionImageLayout(rtImageViews.RTDepthImage, VK_FORMAT_R32_SFLOAT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		VkDescriptorImageInfo colorImageInfo{};
		colorImageInfo.imageView = rtImageViews.RTColorImageView; // Assuming you've created an image view
		colorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkDescriptorImageInfo depthImageInfo{};
		depthImageInfo.imageView = rtImageViews.RTDepthImageView; // Assuming you've created an image view
		depthImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		std::array<VkWriteDescriptorSet, 4> descriptorWrites{};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].pNext = &descriptorAccelerationStructureInfo;
		descriptorWrites[0].dstSet = descriptorSetRT;
		descriptorWrites[0].dstBinding = 0;
		//descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		descriptorWrites[0].descriptorCount = 1;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = descriptorSetRT;
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &bufferInfo;

		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = descriptorSetRT;
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pImageInfo = &colorImageInfo;

		descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[3].dstSet = descriptorSetRT;
		descriptorWrites[3].dstBinding = 3;
		descriptorWrites[3].dstArrayElement = 0;
		descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		descriptorWrites[3].descriptorCount = 1;
		descriptorWrites[3].pImageInfo = &depthImageInfo;

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

		// Get the function pointers required for ray tracing
		vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR"));
		vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR"));
		vkBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device, "vkBuildAccelerationStructuresKHR"));
		vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR"));
		vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR"));
		vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR"));
		vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR"));
		vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR"));
		vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR"));
		vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));

		descriptorLayoutSetupRT();
		createAABBBuffer();
		setupAS();

		createImage(swapChainExtent.width, swapChainExtent.height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT, 
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			rtImageViews.RTColorImage, rtImageViews.RTColorImageMemory);
		transitionImageLayout(rtImageViews.RTColorImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
		rtImageViews.RTColorImageView = createImageView(rtImageViews.RTColorImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

		createImage(swapChainExtent.width, swapChainExtent.height, VK_FORMAT_R32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			rtImageViews.RTDepthImage, rtImageViews.RTDepthImageMemory);
		transitionImageLayout(rtImageViews.RTDepthImage, VK_FORMAT_R32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
		rtImageViews.RTDepthImageView = createImageView(rtImageViews.RTDepthImage, VK_FORMAT_R32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);

		createDescriptorSetsRT(); // needs to be called after images created

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
		hitGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;  // For procedural
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
		rayTracingPipelineCI.groupCount = shaderGroups.size();  // Number of shader groups
		VkRayTracingShaderGroupCreateInfoKHR shaderGroups[] = { raygenGroup, missGroup, hitGroup };
		rayTracingPipelineCI.pGroups = shaderGroups;  // Pointer to shader groups array
		rayTracingPipelineCI.maxPipelineRayRecursionDepth = 1;  // Adjust based on your ray tracing depth requirements
		rayTracingPipelineCI.layout = pipelineLayoutRT;  // The pipeline layout that matches the descriptors used by the shaders
		
		VkResult result = vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayTracingPipelineCI, nullptr, &rayTracingPipeline);
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
		createBuffer(uint64_t(rayTracingPipelineProperties.shaderGroupHandleSize * handleCount),
			VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			shaderBindingTable.buffer, shaderBindingTable.memory,true);
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

		//size (in bytes) of a single shader group handle. This size is provided by the Vulkan ray tracing pipeline properties.
		const uint32_t handleSize = rayTracingPipelineProperties.shaderGroupHandleSize;
		//Shader group handles need to be aligned to a certain size, specified by shaderGroupHandleAlignment.
		//This alignment is necessary for efficient memory access and to meet hardware requirements.
		//align_up is a function that aligns shaderGroupHandleSize to the nearest multiple of shaderGroupHandleAlignment.
		const uint32_t handleSizeAligned = align_up(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);
		const uint32_t groupCount = static_cast<uint32_t>(shaderGroups.size());
		//The total size of the Shader Binding Table is calculated 
		//as the number of shader groups(groupCount) multiplied by the aligned handle size(handleSizeAligned).
		const uint32_t sbtSize = groupCount * handleSizeAligned;

		std::vector<uint8_t> shaderHandleStorage(sbtSize);
		check_vk_result(vkGetRayTracingShaderGroupHandlesKHR(device, rayTracingPipeline, 0, groupCount, sbtSize, shaderHandleStorage.data()));

		shaderBindingTables.raygen.device = &device;
		shaderBindingTables.miss.device = &device;
		shaderBindingTables.hit.device = &device;
		shaderBindingTables.callable.device = &device;

		createShaderBindingTable(shaderBindingTables.raygen, 1);
		createShaderBindingTable(shaderBindingTables.miss, 1);
		createShaderBindingTable(shaderBindingTables.hit, 1);

		// Copy handles
		memcpy(shaderBindingTables.raygen.mapped, shaderHandleStorage.data(), handleSize);
		memcpy(shaderBindingTables.miss.mapped, shaderHandleStorage.data() + handleSizeAligned, handleSize);
		memcpy(shaderBindingTables.hit.mapped, shaderHandleStorage.data() + handleSizeAligned * 2, handleSize);
	}
	
	void MainVulkApplication::createAccelerationStructure(AccelerationStructure& accelerationStructure,
		VkAccelerationStructureTypeKHR type,
		VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo) {

		createBuffer(buildSizeInfo.accelerationStructureSize,
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR,
			accelerationStructure.buffer, accelerationStructure.memory, true
		);
		/*
		// Buffer and memory
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = buildSizeInfo.accelerationStructureSize;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // Assuming exclusive sharing mode
		check_vk_result(vkCreateBuffer(device, &bufferCreateInfo, nullptr, &accelerationStructure.buffer));

		VkMemoryRequirements memoryRequirements{};
		vkGetBufferMemoryRequirements(device, accelerationStructure.buffer, &memoryRequirements);

		VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{};
		memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
		memoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

		VkMemoryAllocateInfo memoryAllocateInfo{};
		memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocateInfo.pNext = &memoryAllocateFlagsInfo;
		memoryAllocateInfo.allocationSize = memoryRequirements.size;
		memoryAllocateInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		check_vk_result(vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &accelerationStructure.memory));
		check_vk_result(vkBindBufferMemory(device, accelerationStructure.buffer, accelerationStructure.memory, 0));
		*/
		// Acceleration structure
		VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
		accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		accelerationStructureCreateInfo.buffer = accelerationStructure.buffer;
		accelerationStructureCreateInfo.size = buildSizeInfo.accelerationStructureSize;
		accelerationStructureCreateInfo.type = type;
		check_vk_result(vkCreateAccelerationStructureKHR(device, &accelerationStructureCreateInfo, nullptr, &accelerationStructure.handle));

		// AS device address
		VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
		accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		accelerationDeviceAddressInfo.accelerationStructure = accelerationStructure.handle;
		accelerationStructure.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(device, &accelerationDeviceAddressInfo);
	}

	void MainVulkApplication::createScratchBuffer(VkDeviceSize size) {

		createBuffer(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR,
			scratchBuffer.handle, scratchBuffer.memory, true);
		/*
		// Buffer and memory
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = size;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // Assuming exclusive sharing mode
		check_vk_result(vkCreateBuffer(device, &bufferCreateInfo, nullptr, &scratchBuffer.handle));

		VkMemoryRequirements memoryRequirements{};
		vkGetBufferMemoryRequirements(device, scratchBuffer.handle, &memoryRequirements);

		VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{};
		memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
		memoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

		VkMemoryAllocateInfo memoryAllocateInfo{};
		memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocateInfo.pNext = &memoryAllocateFlagsInfo;
		memoryAllocateInfo.allocationSize = memoryRequirements.size;
		memoryAllocateInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		check_vk_result(vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &scratchBuffer.memory));
		check_vk_result(vkBindBufferMemory(device, scratchBuffer.handle, scratchBuffer.memory, 0));
		*/
		// Buffer device address
		VkBufferDeviceAddressInfoKHR bufferDeviceAddressInfo{};
		bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		bufferDeviceAddressInfo.buffer = scratchBuffer.handle;
		scratchBuffer.deviceAddress = vkGetBufferDeviceAddressKHR(device, &bufferDeviceAddressInfo);
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

		/*
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, aabbBuffer_, aabbBufferMemory_);
		*/

		createBuffer(bufferSize,
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | 
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
			VK_BUFFER_USAGE_TRANSFER_DST_BIT |
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
			aabbBuffer_, 
			aabbBufferMemory_, 
			true);

		copyBuffer(stagingBuffer, aabbBuffer_, bufferSize);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}
}
#endif
