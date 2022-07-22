#pragma once

#include <map>

#include "VulkanDeviceAccess.hpp"
#include "VulkanEnum.hpp"
#include "VulkanGraphicsPipeline.hpp"
#include "VulkanShaderModule.hpp"

namespace usagi
{
class VulkanGpuDevice;

/*
 * Provide a thin interface for constructing pipeline create info. Most of the
 * elements of the pipeline info are supposed to be set by the client through
 * the reference.
 */
class VulkanGraphicsPipelineCompiler : public VulkanDeviceAccess
{
    vk::GraphicsPipelineCreateInfo mPipelineInfo;
    std::vector<vk::PipelineShaderStageCreateInfo> mShaderStages;
    std::vector<vk::VertexInputBindingDescription> mInputBindings;
    std::vector<vk::VertexInputAttributeDescription> mInputAttributes;
    std::map<std::string, std::uint32_t, std::less<>> mInputAttributeNames;

    struct DescriptorSetBinding
    {
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
    };
    std::map<std::uint32_t, DescriptorSetBinding> mDescriptorBindings;

    struct PushConstantMember
    {
        std::uint32_t offset;
        std::uint32_t size;
        vk::ShaderStageFlagBits stage;
        std::string name;
    };
    std::vector<PushConstantMember> mPushConstantMembers;
    std::vector<vk::PushConstantRange> mPushConstantsRanges;

    void add_input_attribute(std::string name, std::uint32_t location);

    void reflect_vertex_inputs(
        const spirv_cross::Compiler &compiler,
        const spirv_cross::ShaderResources &res);

    void add_descriptor(
        const spirv_cross::Compiler &compiler,
        const spirv_cross::Resource &res,
        vk::ShaderStageFlagBits stage,
        vk::DescriptorType type);

    void warn_ignored_descriptor(
        const spirv_cross::Compiler &compiler,
        const spirv_cross::Resource &res,
        vk::DescriptorType type);

    void reflect_resources(
        const spirv_cross::Compiler &compiler,
        vk::ShaderStageFlagBits stage,
        const spirv_cross::ShaderResources &res);

    void reflect_push_constants(
        const spirv_cross::Compiler &compiler,
        vk::ShaderStageFlagBits stage,
        const spirv_cross::ShaderResources &res);

public:
    vk::GraphicsPipelineCreateInfo & pipeline_info()
    {
        return mPipelineInfo;
    }

    void add_stage_shader(
        Vulkan_GpuShaderStage stage,
        const VulkanShaderModule &module,
        std::string_view entry_func);

    // The user of the pipeline will provide the buffers to the command list,
    // so the structural info should also be provided by the code size instead
    // of by asset files.
    void add_vertex_input_buffer(
        std::uint32_t index,
        std::uint32_t stride,
        Vulkan_GpuVertexInputRate input_rate);

    void add_vertex_input_attribute(
        std::uint32_t location,
        std::uint32_t buffer_index,
        Vulkan_GpuBufferFormat format,
        std::uint32_t offset_in_element);

    void add_vertex_input_attribute(
        std::string_view name,
        std::uint32_t buffer_index,
        Vulkan_GpuBufferFormat format,
        std::uint32_t offset);

    VulkanGraphicsPipeline compile();
};
}
