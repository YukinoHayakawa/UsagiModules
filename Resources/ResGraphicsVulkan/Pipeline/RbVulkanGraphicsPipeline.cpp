#include "RbVulkanGraphicsPipeline.hpp"

#include <iostream>

#include <nlohmann/json.hpp>

#include <Usagi/Modules/Platforms/Vulkan/VulkanGraphicsPipelineCompiler.hpp>
#include <Usagi/Modules/Resources/ResJson/RbCascadingJsonConfig.hpp>
#include <Usagi/Modules/Resources/ResJson/JsonSchemaHelper.hpp>
#include <Usagi/Modules/Runtime/HeapManager/HeapManager.hpp>

#include "RbVulkanShaderModule.hpp"

namespace usagi
{
ResourceState RbVulkanGraphicsPipeline::construct(
    ResourceConstructDelegate<ProductT> &delegate,
    const AssetPath &path)
{
    connect(*delegate.heap<VulkanDeviceAccess *>());

    // Get pipeline description.
    const auto res = delegate.resource<RbCascadingJsonConfig>(path).await();
    const auto &config = *res;

    std::cout << config << std::endl;

    VulkanGraphicsPipelineCompiler compiler;
    compiler.connect(this);

    vk::GraphicsPipelineCreateInfo &pipeline_info = compiler.pipeline_info();

    // =========================== Shader Stages ============================ //

    // todo: support loading SPIR-V

    auto async_shader_module = [&](
        const nlohmann::json &obj,
        const char *key,
        GpuShaderStage stage)
    {
        const auto &asset_path = obj[nlohmann::json::json_pointer(key)];
        CHECK_TYPE(asset_path, key, string)
        // auto src_path = asset_path.get<std::string>();
        auto shader_accessor = delegate.resource<RbVulkanShaderModule>(
            asset_path.get<std::string>(),
            stage
        );
        return shader_accessor;
        // return std::make_pair(std::move(shader), std::move(src_path));
    };

    auto /*[*/future_vert/*, p_vert]*/ = async_shader_module(
        config,
        "/shaders/vertex/source",
        GpuShaderStage::VERTEX
    );
    auto /*[*/future_frag/*, p_frag]*/ = async_shader_module(
        config,
        "/shaders/fragment/source",
        GpuShaderStage::FRAGMENT
    );

    auto module_vert = future_vert.await();
    auto module_frag = future_frag.await();

    std::string entry_vert = config["/shaders/vertex/entry"_json_pointer];
    std::string entry_frag = config["/shaders/fragment/entry"_json_pointer];

    compiler.add_stage_shader(
        GpuShaderStage::VERTEX,
        *module_vert,
        entry_vert
    );
    compiler.add_stage_shader(
        GpuShaderStage::FRAGMENT,
        *module_frag,
        entry_frag
    );

    // ============================ Vertex Input ============================ //

    // Needs app-side input.

    compiler.add_vertex_input_buffer(0, 20, GpuVertexInputRate::VERTEX);
    compiler.add_vertex_input_attribute("inPosition", 0, GpuBufferFormat::R32G32_SFLOAT, 0);
    compiler.add_vertex_input_attribute(1, 0, GpuBufferFormat::R32G32_SFLOAT, 8);

    // =========================== Input Assembly =========================== //

    vk::PipelineInputAssemblyStateCreateInfo input_assembly_state;
    pipeline_info.setPInputAssemblyState(&input_assembly_state);

    FIELD("/input_assembly/primitive_topology",
        input_assembly_state.topology, vk::PrimitiveTopology)
    MATCH("point_list", ePointList)
    ELSE_("line_list", eLineList)
    ELSE_("line_strip", eLineStrip)
    ELSE_("triangle_list", eTriangleList)
    ELSE_("triangle_strip", eTriangleStrip)
    ELSE_("triangle_fan", eTriangleFan)
    ELSE_("line_list_with_adjacency", eLineListWithAdjacency)
    ELSE_("line_strip_with_adjacency", eLineStripWithAdjacency)
    ELSE_("triangle_list_with_adjacency", eTriangleListWithAdjacency)
    ELSE_("triangle_strip_with_adjacency", eTriangleStripWithAdjacency)
    ELSE_("patch_list", ePatchList)
    END_FIELD()

    // ============================ Tessellation ============================ //

    // Tessellation is not enabled so no need to set it up.
    //
    // pTessellationState is a pointer to a
    // VkPipelineTessellationStateCreateInfo structure, and is ignored if the
    // pipeline does not include a tessellation control shader stage and
    // tessellation evaluation shader stage.

    // ============================== Viewport ============================== //

    vk::PipelineViewportStateCreateInfo viewport_state;
    pipeline_info.setPViewportState(&viewport_state);

    // We use VK_DYNAMIC_STATE_VIEWPORT and VK_DYNAMIC_STATE_SCISSOR so the
    // content of this state is ignored.

    // But to keep the validation layer happy:
    viewport_state.setViewportCount(1);
    viewport_state.setScissorCount(1);

    // =========================== Rasterization ============================ //

    vk::PipelineRasterizationStateCreateInfo rasterization_state;
    pipeline_info.setPRasterizationState(&rasterization_state);

    FIELD("/rasterization/polygon_mode",
        rasterization_state.polygonMode, vk::PolygonMode)
    MATCH("fill", eFill)
    ELSE_("line", eLine)
    ELSE_("point", ePoint)
    END_FIELD()

    FIELD("/rasterization/cull_mode",
        rasterization_state.cullMode, vk::CullModeFlagBits)
    MATCH_NULL(eNone)
    MATCH("front", eFront)
    ELSE_("back", eBack)
    ELSE_("all", eFrontAndBack)
    END_FIELD()

    FIELD("/rasterization/front_face",
        rasterization_state.frontFace, vk::FrontFace)
    MATCH("counter_clockwise", eCounterClockwise)
    ELSE_("clockwise", eClockwise)
    END_FIELD()

    // ============================ Multisample ============================= //

    vk::PipelineMultisampleStateCreateInfo multisample_state;
    pipeline_info.setPMultisampleState(&multisample_state);
    multisample_state.setRasterizationSamples(vk::SampleCountFlagBits::e1);
    multisample_state.setMinSampleShading(1.f);

    // Multisampling is not in consideration now.

    // =========================== Depth Stencil ============================ //

    vk::PipelineDepthStencilStateCreateInfo depth_stencil_state;
    pipeline_info.setPDepthStencilState(&depth_stencil_state);

    MATCH_BOOL("/depth_stencil/depth_test",
        depth_stencil_state.depthTestEnable)

    MATCH_BOOL("/depth_stencil/depth_write",
        depth_stencil_state.depthWriteEnable)

    FIELD("/depth_stencil/depth_compare_op",
        depth_stencil_state.depthCompareOp, vk::CompareOp)
    MATCH("false", eNever)
    ELSE_("<", eLess)
    ELSE_("=", eEqual)
    ELSE_("<=", eLessOrEqual)
    ELSE_(">", eGreater)
    ELSE_("!=", eNotEqual)
    ELSE_(">=", eGreaterOrEqual)
    ELSE_("true", eAlways)
    END_FIELD()

    // ============================ Color Blend ============================= //

    vk::PipelineColorBlendStateCreateInfo color_blend_state;
    pipeline_info.setPColorBlendState(&color_blend_state);

    vk::PipelineColorBlendAttachmentState color_blend;
    color_blend_state.setAttachmentCount(1);
    color_blend_state.setPAttachments(&color_blend);

    // =========================== Dynamic State ============================ //

    // ========================== Pipeline Layout =========================== //

    // ============================ Render Pass ============================= //

    // vk::UniqueRenderPass render_pass;


    // Output attachments -> Render pass

    // Shaders

    // Vertex input states (based on reflection info)

    // Pipeline layout (based on reflection info)

    // Descriptor set layout

    // Push constant ranges

    // ============================== Compile =============================== //

    delegate.emplace(compiler.compile());

    return ResourceState::READY;
}
}
