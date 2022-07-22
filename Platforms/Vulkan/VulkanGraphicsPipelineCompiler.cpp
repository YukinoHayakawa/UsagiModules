#include "VulkanGraphicsPipelineCompiler.hpp"

#include <format>
#include <ranges>

#include <Usagi/Modules/Common/Logging/Logging.hpp>

namespace usagi
{
void VulkanGraphicsPipelineCompiler::reflect_vertex_inputs(
    const spirv_cross::Compiler &compiler,
    const spirv_cross::ShaderResources &res)
{
    LOG(debug, "Mapping vertex attributes according to shader reflection");

    for(auto &&r : res.stage_inputs)
    {
        const auto location = compiler.get_decoration(
            r.id,
            spv::DecorationLocation
        );
        add_input_attribute(r.name, location);
    }
}

void VulkanGraphicsPipelineCompiler::add_descriptor(
    const spirv_cross::Compiler &compiler,
    const spirv_cross::Resource &res,
    vk::ShaderStageFlagBits stage,
    vk::DescriptorType type)
{
    const auto set_id = compiler.get_decoration(
        res.id, spv::DecorationDescriptorSet);
    const auto binding = compiler.get_decoration(
        res.id, spv::DecorationBinding);
    const auto array_size = compiler.get_type(res.type_id).vecsize;

    auto &set = mDescriptorBindings[set_id];
    auto binding_it = std::ranges::find_if(
        set.bindings,
        [=](auto &&e) { return e.binding == binding; }
    );
    bool validate = true;
    if(binding_it == set.bindings.end())
    {
        set.bindings.emplace_back();
        binding_it = set.bindings.end() - 1;
        validate = false;
    }

    if(validate)
    {
        USAGI_ASSERT_THROW(
            binding_it->descriptorCount == array_size,
            std::runtime_error(std::format(
                "Descriptor counts don't match in different stages: "
                "set={}, binding={}",
                set_id, binding
            ))
        );
        USAGI_ASSERT_THROW(
            binding_it->descriptorType == type,
            std::runtime_error(std::format(
                "Descriptor types don't match in different stages: "
                "set={}, binding={}",
                set_id, binding
            ))
        );
    }

    binding_it->stageFlags |= stage;
    binding_it->setBinding(binding);
    binding_it->setDescriptorCount(array_size);
    binding_it->setDescriptorType(type);
}

void VulkanGraphicsPipelineCompiler::warn_ignored_descriptor(
    const spirv_cross::Compiler &compiler,
    const spirv_cross::Resource &res,
    [[maybe_unused]] vk::DescriptorType type)
{
    const auto set = compiler.get_decoration(
        res.id, spv::DecorationDescriptorSet);
    const auto binding = compiler.get_decoration(
        res.id, spv::DecorationBinding);

    LOG(warn, "{name} (set={},binding={}) is ignored.",
        res.name, set, binding);
}

void VulkanGraphicsPipelineCompiler::reflect_resources(
    const spirv_cross::Compiler &compiler,
    vk::ShaderStageFlagBits stage,
    const spirv_cross::ShaderResources &resources)
{
    // todo: deal with others resource types

    for(auto &&res : resources.storage_buffers)
        warn_ignored_descriptor(compiler, res,
            vk::DescriptorType::eStorageBuffer);

    // sampler2D is not supported in HLSL so not included here.
    // don't use them in shaders.
    for(auto &&res : resources.sampled_images)
        warn_ignored_descriptor(compiler, res,
            vk::DescriptorType::eSampledImage);

    for(auto &&res : resources.separate_images)
        add_descriptor(compiler, res, stage,
            vk::DescriptorType::eSampledImage);

    for(auto &&res : resources.separate_samplers)
        add_descriptor(compiler, res, stage,
            vk::DescriptorType::eSampler);

    for(auto &&res : resources.uniform_buffers)
        add_descriptor(compiler, res, stage,
            vk::DescriptorType::eUniformBuffer);

    // note that only one subpass is used to maintain compatibility
    // for shader cross-compiling
    for(auto &&res : resources.subpass_inputs)
        add_descriptor(compiler, res, stage,
            vk::DescriptorType::eInputAttachment);
}

void VulkanGraphicsPipelineCompiler::reflect_push_constants(
    const spirv_cross::Compiler &compiler,
    vk::ShaderStageFlagBits stage,
    const spirv_cross::ShaderResources &res)
{
    /*
     * https://github.com/KhronosGroup/Vulkan-Docs/issues/1129
     * https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/vkCmdPushConstants.html
     *
     * Regarding the layout of members in push constant ranges:
     * - If a stage shader wants to access a particular member, the range which
     *   covers that member must have stageFlags set with the corresponding
     *   stage.
     * - If the app is going to update the value of a member, it must pass
     *   the flags of all stages whose constant range declaration included
     *   the memory offset of that member.
     * - Spec also says no two push constant ranges shall include the same
     *   stage.
     * - From the rules above we can conclude that if two stages want to
     *   access a member at the same offset, they will receive the same value.
     *   If they want to read different values, they must use different offsets.
     */
    for(const auto &buf : res.push_constant_buffers)
    {
        const auto &type = compiler.get_type(buf.base_type_id);
        std::uint32_t offset = -1;
        // const auto size = compiler.get_declared_struct_size(type);
        for(unsigned i = 0; i < type.member_types.size(); i++)
        {
            const auto &member_name =
                compiler.get_member_name(type.self, i);
            const auto member_offset =
                compiler.type_struct_member_offset(type, i);
            const auto member_size =
                compiler.get_declared_struct_member_size(type, i);

            offset = std::min(offset, member_offset);

            LOG(debug,
                "Push constant in {} shader: name={}, offset={}, size={}",
                to_string(stage),
                member_name,
                member_offset,
                member_size
            );

            // todo: check if two stage uses the same member they must have same type and name
            // Record the mappings from member name to memory range so it's
            // possible to update push constants by names.
            mPushConstantMembers.push_back({
                .offset = member_offset,
                .size = static_cast<std::uint32_t>(member_size),
                .stage = stage,
                .name = member_name
            });
        }
        const auto size = compiler.get_declared_struct_size(type) - offset;

        LOG(debug, "Push constant range for {} shader: offset={}, size={}",
            to_string(stage), offset, size);

        mPushConstantsRanges.emplace_back(
            stage,
            offset,
            static_cast<std::uint32_t>(size)
        );
    }
}

void VulkanGraphicsPipelineCompiler::add_stage_shader(
    Vulkan_GpuShaderStage stage,
    const VulkanShaderModule &module,
    std::string_view entry_func)
{
    const auto &compiler = module.reflection();
    const auto res = compiler.get_shader_resources();

    if(stage == vk::ShaderStageFlagBits::eVertex)
        reflect_vertex_inputs(compiler, res);
    reflect_resources(compiler, stage, res);
    reflect_push_constants(compiler, stage, res);

    mShaderStages.push_back(
        { { }, stage, module.shader_module().get(), entry_func.data() }
    );
}

void VulkanGraphicsPipelineCompiler::add_input_attribute(
    std::string name,
    std::uint32_t location)
{
    LOG(debug, "Mapping location {} to name {}", location, name);
    auto [it, inserted] = mInputAttributeNames.insert(
        { std::move(name), location });
    USAGI_ASSERT_THROW(inserted, std::runtime_error(std::format(
        "Inserting duplicated key: {}", it->first
    )));
}

void VulkanGraphicsPipelineCompiler::add_vertex_input_buffer(
    std::uint32_t index,
    std::uint32_t stride,
    Vulkan_GpuVertexInputRate input_rate)
{
    mInputBindings.emplace_back(index, stride, input_rate);
}

void VulkanGraphicsPipelineCompiler::add_vertex_input_attribute(
    std::uint32_t location,
    std::uint32_t buffer_index,
    Vulkan_GpuBufferFormat format,
    std::uint32_t offset_in_element)
{
    LOG(debug,
        "Addind vertex input attribute: location={}, binding={}, "
        "format={}, offset={}",
        location,
        buffer_index,
        to_string(format),
        offset_in_element
    );

    mInputAttributes.emplace_back(
        location,
        buffer_index,
        format,
        offset_in_element
    );
}

void VulkanGraphicsPipelineCompiler::add_vertex_input_attribute(
    std::string_view name,
    std::uint32_t buffer_index,
    Vulkan_GpuBufferFormat format,
    std::uint32_t offset)
{
    const auto it = mInputAttributeNames.find(name);
    USAGI_ASSERT_THROW(
        it != mInputAttributeNames.end(),
        std::runtime_error(std::format(
            "Vertex attribute not found: {}}", name
        )));
    add_vertex_input_attribute(it->second, buffer_index, format, offset);
}

VulkanGraphicsPipeline VulkanGraphicsPipelineCompiler::compile()
{
    mPipelineInfo.setFlags(vk::PipelineCreateFlagBits::eAllowDerivatives);

    // =========================== Shader Stages ============================ //

    mPipelineInfo.setStages(mShaderStages);

    // ============================ Vertex Input ============================ //

    vk::PipelineVertexInputStateCreateInfo vertex_input_state;
    mPipelineInfo.setPVertexInputState(&vertex_input_state);

    /*
    * The location of a vertex input variable is the index for accessing the
    * variable.
    *
    * layout(location = 0) in vec2 inPosition;
    * layout(location = 1) in vec3 inColor;
    *
    * A binding describes the traits of the buffer that will be provided
    * when calling `vkCmdBindVertexBuffers`, including how to divide the data
    * into elements (stride) and how often the vertex input is updated
    * (inputRate). The binding number is used by input attributes to locate the
    * data it needs in the correct buffer.
    *
    * typedef struct VkVertexInputBindingDescription {
    *     uint32_t             binding;
    *     uint32_t             stride;
    *     VkVertexInputRate    inputRate;
    * } VkVertexInputBindingDescription;
    *
    * This struct describes how the data is feed to the an input attribute.
    *
    * typedef struct VkVertexInputAttributeDescription {
    *     uint32_t    location;  <- correspond to the layout qualifier value
    *     uint32_t    binding;   <- specify which buffer to read from
    *     VkFormat    format;    <- specify the source format
    *     uint32_t    offset;    <- specify the offset in the source element
    * } VkVertexInputAttributeDescription;
    *
    * Some refs:
    * https://vulkan.lunarg.com/doc/view/1.2.182.0/windows/chunked_spec/chap22.html
    * https://stackoverflow.com/questions/40450342/what-is-the-purpose-of-binding-from-vkvertexinputbindingdescription
    */

    vertex_input_state.setVertexBindingDescriptions(mInputBindings);
    vertex_input_state.setVertexAttributeDescriptions(mInputAttributes);

    // The following empty sections are supposed to be externally configured.

    // =============================== BEGIN ================================ //

    // =========================== Input Assembly =========================== //
    //
    // ============================ Tessellation ============================ //
    //
    // ============================== Viewport ============================== //
    //
    // =========================== Rasterization ============================ //
    //
    // ============================ Multisample ============================= //
    //
    // =========================== Depth Stencil ============================ //
    //
    // ============================ Color Blend ============================= //

    // ================================ END ================================= //

    // =========================== Dynamic State ============================ //

    auto dynamic_states = {
        vk::DynamicState::eScissor,
        vk::DynamicState::eViewport,
        vk::DynamicState::eLineWidth
    };
    vk::PipelineDynamicStateCreateInfo dynamic_state;
    mPipelineInfo.setPDynamicState(&dynamic_state);
    dynamic_state.setDynamicStates(dynamic_states);

    // ========================== Pipeline Layout =========================== //

    // todo: should these var be preserved after creating the pipeline? maybe
    std::vector<VulkanUniqueDescriptorSetLayout> descriptor_set_layouts;
    std::vector<vk::DescriptorSetLayout> descriptor_set_layout_refs;

    for(auto &[bindings] : mDescriptorBindings | std::views::values)
    {
        vk::DescriptorSetLayoutCreateInfo set_layout_info;
        set_layout_info.setBindings(bindings);
        descriptor_set_layouts.emplace_back(
            // todo these should be stored in pipeline
            create(set_layout_info)
        );
        descriptor_set_layout_refs.emplace_back(
            descriptor_set_layouts.back().get()
        );
    }

    vk::PipelineLayoutCreateInfo pipeline_layout_info;
    pipeline_layout_info.setPushConstantRanges(mPushConstantsRanges);
    pipeline_layout_info.setSetLayouts(descriptor_set_layout_refs);
    auto compatible_pipeline_layout = create(pipeline_layout_info);
    mPipelineInfo.setLayout(compatible_pipeline_layout.get());

    // ============================ Render Pass ============================= //

    vk::RenderPassCreateInfo render_pass_info;

    std::vector<vk::AttachmentDescription> buf_desc;
    // attachment_descriptions.reserve(info.attachment_usages.size());
    // for(auto &&u : info.attachment_usages)
    {
        vk::AttachmentDescription d;
        d.setFormat(vk::Format::eB8G8R8A8Snorm);
        d.setSamples(vk::SampleCountFlagBits::e1);
        d.setInitialLayout(vk::ImageLayout::eUndefined);
        d.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);
        d.setLoadOp(vk::AttachmentLoadOp::eDontCare);
        d.setStoreOp(vk::AttachmentStoreOp::eStore);
        buf_desc.push_back(d);
    }
    render_pass_info.setAttachments(buf_desc);

    std::vector<vk::SubpassDescription> subpasses;
    std::vector<vk::AttachmentReference> buf_refs;
    // vk::AttachmentReference ds_ref;
    // note: only one depth stencil attachment is allowed in one framebuffer - configure in pipeline?
    // for(std::size_t i = 0; i < info.attachment_usages.size(); ++i)
    {
        // auto &&a = info.attachment_usages[i];
        // if(a.layout == GpuImageLayout::DEPTH_STENCIL_ATTACHMENT)
        // {
            // if(subpass.pDepthStencilAttachment)
                // LOG(error, "Only one depth stencil attachment may be used.");
            // ds_ref.setAttachment(static_cast<uint32_t>(i));
            // ds_ref.setLayout(translate(a.layout));
            // subpass.setPDepthStencilAttachment(&ds_ref);
        // }
        // else
        // reflect on shaders? but can't know the layout, so should be provided by the app side
        {
            vk::AttachmentReference r;
            r.setAttachment(0);
            r.setLayout(vk::ImageLayout::eColorAttachmentOptimal);
            buf_refs.push_back(r);
        }
    }
    vk::SubpassDescription subpass;
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
    subpass.setColorAttachments(buf_refs);
    subpasses.push_back(subpass);

    render_pass_info.setSubpasses(subpasses);

    auto compatible_render_pass = create(render_pass_info);

    mPipelineInfo.setRenderPass(compatible_render_pass.get());

    // todo check result
    auto [pipeline, result] = create(mPipelineInfo);

    return {
        std::move(descriptor_set_layouts),
        std::move(compatible_pipeline_layout),
        std::move(compatible_render_pass),
        std::move(pipeline)
    };
}
}
