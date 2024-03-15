#ifndef __VK_LIGHTDEPTH_HPP__
#define __VK_LIGHTDEPTH_HPP__

namespace VkApplication {

	void MainVulkApplication::LightDepthDraw() {

        vkWaitForFences(device, 1, &LightDepthFence, VK_TRUE, UINT64_MAX);

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        check_vk_result(vkBeginCommandBuffer(LightDepthCommandBuffer, &beginInfo));

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPassLightDepth;
        renderPassInfo.framebuffer = LightDepthFramebuffer;
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = { (uint32_t)WIDTH_SHADOWMAP, (uint32_t) HEIGHT_SHADOWMAP };

        std::array<VkClearValue, 2> clearValues{};
        //std::array<VkClearValue, 1> clearValues{};
        clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        clearValues[1].depthStencil = { 1.0f, 0 };

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(LightDepthCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindDescriptorSets(LightDepthCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, LightDepthPipelineLayout, 0, 1, &descriptorSetLightDepth, 0, NULL);
        vkCmdBindPipeline(LightDepthCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, LightDepthPipeline);

        VkBuffer vertexBuffers[] = { vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(LightDepthCommandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(LightDepthCommandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        for (const auto& objectName : objectsToRenderForFrame) {

            auto& offsets = objectHash[objectName];
            uint32_t indexOffset = offsets.first;
            uint32_t indexCount = offsets.second; // Adjust based on your actual structure

            vkCmdDrawIndexed(LightDepthCommandBuffer, indexCount, 1, indexOffset, 0, 0);
        }

        VkBuffer vertexBuffersGround[] = { vertexBuffer_ground };
        vkCmdBindVertexBuffers(LightDepthCommandBuffer, 0, 1, vertexBuffersGround, offsets);
        vkCmdBindIndexBuffer(LightDepthCommandBuffer, indexBuffer_ground, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(LightDepthCommandBuffer, static_cast<uint32_t>(indices_ground.size()), 1, 0, 0, 0);

        VkBuffer vertexBuffersAvatar[] = { vertexBuffer_avatar };
        vkCmdBindVertexBuffers(LightDepthCommandBuffer, 0, 1, vertexBuffersAvatar, offsets);
        vkCmdBindIndexBuffer(LightDepthCommandBuffer, indexBuffer_avatar, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(LightDepthCommandBuffer, static_cast<uint32_t>(indices_avatar.size()), 1, 0, 0, 0);

        vkCmdEndRenderPass(LightDepthCommandBuffer);

        if (vkEndCommandBuffer(LightDepthCommandBuffer) != VK_SUCCESS)
            throw std::runtime_error("failed to record command buffer!");

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        // If you need to wait on any semaphores before starting the G-buffer pass
        VkSemaphore waitSemaphores{};
        VkPipelineStageFlags waitStages{ };
        submitInfo.waitSemaphoreCount = 0; // Update this count appropriately
        submitInfo.pWaitSemaphores = &waitSemaphores;
        submitInfo.pWaitDstStageMask = &waitStages;

        VkSemaphore signalSemaphores[] = { LightDepthCompleteSemaphore };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        // Setting the command buffer for the G-buffer pass
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &LightDepthCommandBuffer;

        vkResetFences(device, 1, &LightDepthFence);

        check_vk_result(vkQueueSubmit(graphicsQueue, 1, &submitInfo, LightDepthFence));
	}

	void MainVulkApplication::LightDepthRenderPipelineSetup() {

        createImage(WIDTH_SHADOWMAP, HEIGHT_SHADOWMAP, VK_FORMAT_R32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            textureImage_lightDepth, textureImageMemory_lightDepth);
        textureImageView_lightDepth = createImageView(textureImage_lightDepth, VK_FORMAT_R32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT);

        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        std::array<VkDescriptorSetLayoutBinding, 1> bindings = { uboLayoutBinding };
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayoutLightDepth) != VK_SUCCESS) 
            throw std::runtime_error("failed to create descriptor set layout!");
        
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descriptorSetLayoutLightDepth;

        if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSetLightDepth) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate descriptor sets!");

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[0];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSetLightDepth;
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = findDepthFormat();
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 0;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // Define the subpass description
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;  // Bind point for the subpass
        subpass.colorAttachmentCount = 0;                             // Number of color attachments
        subpass.pDepthStencilAttachment = &depthAttachmentRef;                    // No depth/stencil attachment

        // Define the subpass dependency
        VkSubpassDependency subpassDependency = {};
        subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;           // Implicit subpass before the render pass
        subpassDependency.dstSubpass = 0;                              // Subpass index of this render pass
        subpassDependency.srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        subpassDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        subpassDependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        subpassDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        // Fill in the reflectRenderPassInfo structure
        VkAttachmentDescription attachments[] = { depthAttachment };
        VkRenderPassCreateInfo RenderPassInfo = {};
        RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        RenderPassInfo.attachmentCount = 1;                     // Number of attachments
        RenderPassInfo.pAttachments = attachments;         // Attachment description(s)
        RenderPassInfo.subpassCount = 1;                        // Number of subpasses
        RenderPassInfo.pSubpasses = &subpass;                   // Subpass description(s)
        RenderPassInfo.dependencyCount = 1;                     // Number of subpass dependencies
        RenderPassInfo.pDependencies = &subpassDependency;       // Subpass dependency(ies)

        if (vkCreateRenderPass(device, &RenderPassInfo, nullptr, &renderPassLightDepth) != VK_SUCCESS)
            throw std::runtime_error("failed to create render pass!");

        auto vertShaderCode = readFile("shaders/lightdepth_vert.spv");
        auto fragShaderCode = readFile("shaders/lightdepth_frag.spv");

        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        std::vector<VkVertexInputBindingDescription> bindingDescriptions;
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

        // Vertex input bindings
        // The instancing pipeline uses a vertex input state with two bindings
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

        VkViewport viewport;
        viewport.x = 0.0f;                              // X offset of the viewport
        viewport.y = 0.0f;                              // Y offset of the viewport
        viewport.width = (float)WIDTH_SHADOWMAP;
        viewport.height = (float)HEIGHT_SHADOWMAP;
        viewport.minDepth = 0.0f;                       // Minimum depth value
        viewport.maxDepth = 1.0f;                       // Maximum depth value

        VkRect2D scissor;
        scissor.offset = { 0, 0 };                      // Offset of the scissor rectangle
        scissor.extent = { (unsigned int)WIDTH_SHADOWMAP,(unsigned int)HEIGHT_SHADOWMAP }; // Extent of the scissor rectangle

        // Fill in the reflectViewportStateInfo structure
        VkPipelineViewportStateCreateInfo viewportStateInfo = {};
        viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateInfo.pNext = nullptr;
        viewportStateInfo.flags = 0;
        viewportStateInfo.viewportCount = 1;            // Number of viewports
        viewportStateInfo.pViewports = &viewport;
        viewportStateInfo.scissorCount = 1;             // Number of scissors
        viewportStateInfo.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        //rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // Fill in the reflectDepthStencilInfo structure
        VkPipelineDepthStencilStateCreateInfo LightDepthStencilInfo = {};
        LightDepthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        LightDepthStencilInfo.pNext = nullptr;
        LightDepthStencilInfo.flags = 0;
        LightDepthStencilInfo.depthTestEnable = VK_TRUE;              // Enable depth testing
        LightDepthStencilInfo.depthWriteEnable = VK_TRUE;             // Enable writing to the depth buffer
        LightDepthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;    // Depth comparison operation (adjust as needed)
        LightDepthStencilInfo.depthBoundsTestEnable = VK_FALSE;       // Disable depth bounds testing
        LightDepthStencilInfo.stencilTestEnable = VK_FALSE;           // Disable stencil testing
        LightDepthStencilInfo.front = {};                             // Stencil operations for front-facing polygons
        LightDepthStencilInfo.back = {};                              // Stencil operations for back-facing polygons
        LightDepthStencilInfo.minDepthBounds = 0.0f;                  // Minimum depth bound
        LightDepthStencilInfo.maxDepthBounds = 1.0f;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayoutLightDepth;

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &LightDepthPipelineLayout) != VK_SUCCESS)
            throw std::runtime_error("failed to create pipeline layout!");

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportStateInfo;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &LightDepthStencilInfo;
        pipelineInfo.layout = LightDepthPipelineLayout;
        pipelineInfo.renderPass = renderPassLightDepth;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &LightDepthPipeline) != VK_SUCCESS)
            throw std::runtime_error("failed to create graphics pipeline!");

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &LightDepthCompleteSemaphore) != VK_SUCCESS)
            throw std::runtime_error("Failed to create semaphores!");

        check_vk_result(vkCreateFence(device, &fenceInfo, nullptr, &LightDepthFence));

        VkImageView attachmentsView[] = { textureImageView_lightDepth };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPassLightDepth;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachmentsView;
        framebufferInfo.width = WIDTH_SHADOWMAP;
        framebufferInfo.height = HEIGHT_SHADOWMAP;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &LightDepthFramebuffer) != VK_SUCCESS)
            throw std::runtime_error("failed to create framebuffer!");

        VkCommandBufferAllocateInfo commandBufferAllocInfo{};
        commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocInfo.commandPool = commandPool;
        commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(device, &commandBufferAllocInfo, &LightDepthCommandBuffer) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate command buffers!");

	}
}

#endif