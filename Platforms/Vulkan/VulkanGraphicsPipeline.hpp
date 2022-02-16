#pragma once

#include <vector>

#include "Vulkan.hpp"

namespace usagi
{
class VulkanGraphicsPipeline
{
    std::vector<VulkanUniqueDescriptorSetLayout> mDescriptorSetLayouts;
    VulkanUniquePipelineLayout mCompatiblePipelineLayout;
    VulkanUniqueRenderPass mCompatibleRenderPass;
    VulkanUniquePipeline mPipeline;

public:
    VulkanGraphicsPipeline(
        std::vector<VulkanUniqueDescriptorSetLayout> descriptor_set_layouts,
        VulkanUniquePipelineLayout compatible_pipeline_layout,
        VulkanUniqueRenderPass compatible_render_pass,
        VulkanUniquePipeline pipeline)
        : mDescriptorSetLayouts(std::move(descriptor_set_layouts))
        , mCompatiblePipelineLayout(std::move(compatible_pipeline_layout))
        , mCompatibleRenderPass(std::move(compatible_render_pass))
        , mPipeline(std::move(pipeline))
    {
    }

    const auto & descriptor_set_layouts() const
    {
        return mDescriptorSetLayouts;
    }

    auto compatible_pipeline_layout() const
    {
        return mCompatiblePipelineLayout.get();
    }

    auto compatible_render_pass() const
    {
        return mCompatibleRenderPass.get();
    }

    auto pipeline() const
    {
        return mPipeline.get();
    }
};
}
