#pragma once

#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>

#include <GLFW/glfw3.h>

#include "VkBootstrap.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

#include "bootstrap.hpp"
#include "shaders.hpp"

namespace retort {

const size_t MAXIMUM_FRAMES_IN_FLIGHT = 12;

std::vector<char> read_file(const char *filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);
  if (!file.is_open()) {
    PANIC("EXPLODE");
  }

  size_t file_size = file.tellg();
  std::vector<char> buffer(file_size);
  file.seekg(0);
  file.read(buffer.data(), (std::streamsize)file_size);
  file.close();

  return buffer;
}

struct RenderData {
  VkQueue graphics_queue;
  VkQueue present_queue;
  VkPipelineLayout pipeline_layout;
  VkRenderPass render_pass;
  VkPipeline graphics_pipeline;

  std::vector<VkImage> swapchain_images;
  std::vector<VkImageView> swapchain_image_views;
  std::vector<VkFramebuffer> framebuffers;

  VkCommandPool command_pool;
  std::vector<VkCommandBuffer> command_buffers;

  std::vector<VkSemaphore> available_semaphores;
  std::vector<VkSemaphore> finished_semaphore;
  std::vector<VkFence> in_flight_fences;
  std::vector<VkFence> image_in_flight;
  size_t current_frame = 0;
};

struct Renderer {
  GLFWwindow *window;
  vkb::Device device;
  vkb::Swapchain swapchain;
  vkb::DispatchTable dispatch;

  RenderData render_data;

  vkb::Result<vkb::Swapchain> create_swapchain(
      std::optional<std::reference_wrapper<vkb::Swapchain>> old_swapchain =
          std::nullopt) {
    vkb::SwapchainBuilder swapchain_builder(device);
    if (old_swapchain)
      swapchain_builder.set_old_swapchain(*old_swapchain);
    auto swap_ret = swapchain_builder.build();
    if (old_swapchain)
      vkb::destroy_swapchain(*old_swapchain);
    return swap_ret;
  }

  VkResult create_queues() {
    auto graphics_queue = device.get_queue(vkb::QueueType::graphics);
    if (!graphics_queue.has_value())
      PANIC("NO GRAPHICS QUEUE");
    render_data.graphics_queue = graphics_queue.value();

    auto present_queue = device.get_queue(vkb::QueueType::present);
    if (!present_queue.has_value())
      PANIC("NO PRESENT QUEUE");
    render_data.present_queue = present_queue.value();

    return VK_SUCCESS;
  }

  VkResult create_render_pass() {
    VkAttachmentDescription color_attachment = {};
    color_attachment.format = swapchain.image_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                               VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    if (dispatch.createRenderPass(&render_pass_info, nullptr,
                                  &render_data.render_pass)) {
      PANIC("RENDERPASS CREATION FAILED");
    }

    return VK_SUCCESS;
  }

  VkShaderModule create_shader_module(const uint32_t *code, size_t len) {
    VkShaderModuleCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = len;
    create_info.pCode = code;

    VkShaderModule shader_module;
    CHECK_VK_ERRC(
        dispatch.createShaderModule(&create_info, nullptr, &shader_module));

    return shader_module;
  }

  VkShaderModule create_shader_module(std::vector<char> code) {
    return create_shader_module((uint32_t *)code.data(), code.size());
  }

  VkResult create_graphics_pipeline() {
    auto compiler = Compiler();

    auto vert_code = compiler.compile("<inline vertex shader>",
                                      shaderc_vertex_shader, vertex_shader);
    auto frag_code = read_file("frag.spv");

    VkShaderModule vert_module =
        create_shader_module(vert_code.data(), vert_code.size() * 4);
    VkShaderModule frag_module = create_shader_module(frag_code);

    if (vert_module == VK_NULL_HANDLE || frag_module == VK_NULL_HANDLE) {
      PANIC("no shader modules :c");
    }

    VkPipelineShaderStageCreateInfo vert_stage_info = {};
    vert_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_stage_info.module = vert_module;
    vert_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_stage_info = {};
    frag_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_stage_info.module = frag_module;
    frag_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {vert_stage_info,
                                                       frag_stage_info};

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
    vertex_input_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = 0;
    vertex_input_info.vertexAttributeDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
    input_assembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.;
    viewport.y = 0.;
    viewport.width = swapchain.extent.width;
    viewport.height = swapchain.extent.height;
    viewport.minDepth = 0.;
    viewport.maxDepth = 1.;

    VkRect2D scissors = {};
    scissors.offset = {0, 0};
    scissors.extent = swapchain.extent;

    VkPipelineViewportStateCreateInfo viewport_state = {};
    viewport_state.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &scissors;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo color_blending = {};
    color_blending.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = VK_LOGIC_OP_COPY;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &colorBlendAttachment;
    color_blending.blendConstants[0] = 0.0f;
    color_blending.blendConstants[1] = 0.0f;
    color_blending.blendConstants[2] = 0.0f;
    color_blending.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipeline_layout_info = {};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 0;
    pipeline_layout_info.pushConstantRangeCount = 0;

    CHECK_VK_ERRC(dispatch.createPipelineLayout(&pipeline_layout_info, nullptr,
                                                &render_data.pipeline_layout));

    std::vector<VkDynamicState> dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT,
                                                  VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamic_info = {};
    dynamic_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_info.dynamicStateCount =
        static_cast<uint32_t>(dynamic_states.size());
    dynamic_info.pDynamicStates = dynamic_states.data();

    VkGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDynamicState = &dynamic_info;
    pipeline_info.layout = render_data.pipeline_layout;
    pipeline_info.renderPass = render_data.render_pass;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

    CHECK_VK_ERRC(dispatch.createGraphicsPipelines(
        VK_NULL_HANDLE, 1, &pipeline_info, nullptr,
        &render_data.graphics_pipeline));

    dispatch.destroyShaderModule(frag_module, nullptr);
    dispatch.destroyShaderModule(vert_module, nullptr);
    return VK_SUCCESS;
  }

  VkResult create_framebuffers() {
    render_data.swapchain_images = swapchain.get_images().value();
    render_data.swapchain_image_views = swapchain.get_image_views().value();

    render_data.framebuffers.resize(render_data.swapchain_image_views.size());
    for (size_t i = 0; i < render_data.swapchain_image_views.size(); i++) {
      VkImageView attachments[] = {render_data.swapchain_image_views[i]};

      VkFramebufferCreateInfo framebuffer_info = {};
      framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebuffer_info.renderPass = render_data.render_pass;
      framebuffer_info.attachmentCount = 1;
      framebuffer_info.pAttachments = attachments;
      framebuffer_info.width = swapchain.extent.width;
      framebuffer_info.height = swapchain.extent.height;
      framebuffer_info.layers = 1;

      CHECK_VK_ERRC(dispatch.createFramebuffer(&framebuffer_info, nullptr,
                                               &render_data.framebuffers[i]));
    }

    return VK_SUCCESS;
  }

  VkResult create_sync_objects() {
    render_data.available_semaphores.resize(MAXIMUM_FRAMES_IN_FLIGHT);
    render_data.finished_semaphore.resize(MAXIMUM_FRAMES_IN_FLIGHT);
    render_data.in_flight_fences.resize(MAXIMUM_FRAMES_IN_FLIGHT);
    render_data.image_in_flight.resize(swapchain.image_count, VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAXIMUM_FRAMES_IN_FLIGHT; i++) {
      CHECK_VK_ERRC(dispatch.createSemaphore(
          &semaphore_info, nullptr, &render_data.available_semaphores[i]));
      CHECK_VK_ERRC(dispatch.createSemaphore(
          &semaphore_info, nullptr, &render_data.finished_semaphore[i]));
      CHECK_VK_ERRC(dispatch.createFence(&fence_info, nullptr,
                                         &render_data.in_flight_fences[i]));
    }

    return VK_SUCCESS;
  }

  VkResult create_command_pool() {
    VkCommandPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex =
        device.get_queue_index(vkb::QueueType::graphics).value();

    CHECK_VK_ERRC(dispatch.createCommandPool(&pool_info, nullptr,
                                             &render_data.command_pool));
    return VK_SUCCESS;
  }

  VkResult create_command_buffers() {
    render_data.command_buffers.resize(render_data.framebuffers.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = render_data.command_pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)render_data.command_buffers.size();

    CHECK_VK_ERRC(dispatch.allocateCommandBuffers(
        &allocInfo, render_data.command_buffers.data()));

    for (size_t i = 0; i < render_data.command_buffers.size(); i++) {
      VkCommandBufferBeginInfo begin_info = {};
      begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

      CHECK_VK_ERRC(dispatch.beginCommandBuffer(render_data.command_buffers[i],
                                                &begin_info));

      VkRenderPassBeginInfo render_pass_info = {};
      render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      render_pass_info.renderPass = render_data.render_pass;
      render_pass_info.framebuffer = render_data.framebuffers[i];
      render_pass_info.renderArea.offset = {0, 0};
      render_pass_info.renderArea.extent = swapchain.extent;
      VkClearValue clearColor{{{0.0f, 0.0f, 0.0f, 1.0f}}};
      render_pass_info.clearValueCount = 1;
      render_pass_info.pClearValues = &clearColor;

      VkViewport viewport = {};
      viewport.x = 0.0f;
      viewport.y = 0.0f;
      viewport.width = (float)swapchain.extent.width;
      viewport.height = (float)swapchain.extent.height;
      viewport.minDepth = 0.0f;
      viewport.maxDepth = 1.0f;

      VkRect2D scissor = {};
      scissor.offset = {0, 0};
      scissor.extent = swapchain.extent;

      dispatch.cmdSetViewport(render_data.command_buffers[i], 0, 1, &viewport);
      dispatch.cmdSetScissor(render_data.command_buffers[i], 0, 1, &scissor);

      dispatch.cmdBeginRenderPass(render_data.command_buffers[i],
                                  &render_pass_info,
                                  VK_SUBPASS_CONTENTS_INLINE);

      dispatch.cmdBindPipeline(render_data.command_buffers[i],
                               VK_PIPELINE_BIND_POINT_GRAPHICS,
                               render_data.graphics_pipeline);

      dispatch.cmdDraw(render_data.command_buffers[i], 4, 1, 0, 0);

      dispatch.cmdEndRenderPass(render_data.command_buffers[i]);

      CHECK_VK_ERRC(dispatch.endCommandBuffer(render_data.command_buffers[i]));
    }
    return VK_SUCCESS;
  }

  VkResult recreate_swapchain() {
    dispatch.deviceWaitIdle();
    dispatch.destroyCommandPool(render_data.command_pool, nullptr);

    for (auto framebuffer : render_data.framebuffers) {
      dispatch.destroyFramebuffer(framebuffer, nullptr);
    }

    swapchain.destroy_image_views(render_data.swapchain_image_views);

    this->swapchain = create_swapchain(swapchain).value();
    create_framebuffers();
    create_command_pool();
    create_command_buffers();
    return VK_SUCCESS;
  }

  VkResult draw_frame() {
    dispatch.waitForFences(
        1, &render_data.in_flight_fences[render_data.current_frame], VK_TRUE,
        UINT64_MAX);

    uint32_t image_index = 0;
    VkResult result = dispatch.acquireNextImageKHR(
        swapchain, UINT64_MAX,
        render_data.available_semaphores[render_data.current_frame],
        VK_NULL_HANDLE, &image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      return recreate_swapchain();
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
      CHECK_VK_ERRC(result);
    }

    if (render_data.image_in_flight[image_index] != VK_NULL_HANDLE) {
      dispatch.waitForFences(1, &render_data.image_in_flight[image_index],
                             VK_TRUE, UINT64_MAX);
    }
    render_data.image_in_flight[image_index] =
        render_data.in_flight_fences[render_data.current_frame];

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore wait_semaphores[] = {
        render_data.available_semaphores[render_data.current_frame]};
    VkPipelineStageFlags wait_stages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = wait_semaphores;
    submitInfo.pWaitDstStageMask = wait_stages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &render_data.command_buffers[image_index];

    VkSemaphore signal_semaphores[] = {
        render_data.finished_semaphore[render_data.current_frame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signal_semaphores;

    dispatch.resetFences(
        1, &render_data.in_flight_fences[render_data.current_frame]);

    CHECK_VK_ERRC(dispatch.queueSubmit(
        render_data.graphics_queue, 1, &submitInfo,
        render_data.in_flight_fences[render_data.current_frame]));

    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;

    VkSwapchainKHR swapChains[] = {swapchain};
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapChains;

    present_info.pImageIndices = &image_index;

    result = dispatch.queuePresentKHR(render_data.present_queue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
      return recreate_swapchain();
    } else {
      CHECK_VK_ERRC(result);
    }

    render_data.current_frame =
        (render_data.current_frame + 1) % MAXIMUM_FRAMES_IN_FLIGHT;
    return VK_SUCCESS;
  }

  Renderer(Bootstrap bootstrap) {
    this->window = bootstrap.window;
    this->device = bootstrap.device;
    this->dispatch = bootstrap.device.make_table();

    this->swapchain = create_swapchain().value();
    CHECK_VK_ERRC(create_queues());
    CHECK_VK_ERRC(create_render_pass());
    CHECK_VK_ERRC(create_graphics_pipeline());
    CHECK_VK_ERRC(create_framebuffers());
    CHECK_VK_ERRC(create_command_pool());
    CHECK_VK_ERRC(create_command_buffers());
    CHECK_VK_ERRC(create_sync_objects());
  }
};

} // namespace retort