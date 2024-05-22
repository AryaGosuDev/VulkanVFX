#ifndef __VK_RT_HPP__
#define __VK_RT_HPP__


namespace VkApplication {

	void MainVulkApplication::DrawRT() {

	}

	void MainVulkApplication::pipelineLayoutSetupRT() {

		VkDescriptorSetLayoutBinding uniformLayoutBinding{};
		uniformLayoutBinding.binding = 1;
		uniformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformLayoutBinding.descriptorCount = 1;
		uniformLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR;

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		VkDescriptorSetLayoutBinding bindings[] = {uniformLayoutBinding };
		layoutInfo.pBindings = bindings;

		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayoutRT) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor set layout!");
		}

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;  // Use more if you have multiple descriptor set layouts
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayoutRT;
		pipelineLayoutInfo.pushConstantRangeCount = 0;  // Add push constant ranges if needed

		check_vk_result(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayoutRT));
	}

	void MainVulkApplication::setupRT() {

		auto rayGShaderCode = readFile("shaders/shader_base_vert.spv");
		auto missRTShaderCode = readFile("shaders/shader_base_frag.spv");
		auto closeHitRTShaderCode = readFile("shaders/shader_base_frag.spv");
		auto intersectRTShaderCode = readFile("shaders/shader_base_frag.spv");

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

		VkRayTracingShaderGroupCreateInfoKHR missGroup{};
		missGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		missGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;  // General type for miss shaders
		missGroup.generalShader = 1;  // Index in pStages array
		missGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
		missGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		missGroup.intersectionShader = VK_SHADER_UNUSED_KHR;

		VkRayTracingShaderGroupCreateInfoKHR hitGroup{};
		hitGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		hitGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;  // For triangle geometry
		hitGroup.generalShader = VK_SHADER_UNUSED_KHR;  // Not used in hit groups
		hitGroup.closestHitShader = 2;  // Index in pStages array
		hitGroup.anyHitShader = VK_SHADER_UNUSED_KHR;  // Optional: VK_SHADER_UNUSED_KHR if not used
		hitGroup.intersectionShader = 3;  // Not used for triangles

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

		check_vk_result(vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE,1, &rayTracingPipelineCI, nullptr, &rayTracingPipeline));

	}


}
#endif
