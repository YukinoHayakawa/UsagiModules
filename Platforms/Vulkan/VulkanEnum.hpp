#pragma once

#include <utility>

#include <vulkan/vulkan.hpp>

#include <Usagi/Library/Utilities/EnumTranslation.hpp>
#include <Usagi/Modules/IO/Graphics/Enum.hpp>

namespace usagi
{
template <>
struct EnumMapping<GpuImageLayout, vk::ImageLayout>
{
    constexpr static auto MAPPING = {
        std::pair { GpuImageLayout::UNDEFINED, vk::ImageLayout::eUndefined },
        std::pair { GpuImageLayout::PRESENT, vk::ImageLayout::ePresentSrcKHR },
        std::pair { GpuImageLayout::TRANSFER_SRC, vk::ImageLayout::eTransferSrcOptimal },
        std::pair { GpuImageLayout::TRANSFER_DST, vk::ImageLayout::eTransferDstOptimal },
        std::pair { GpuImageLayout::ATTACHMENT, vk::ImageLayout::eAttachmentOptimalKHR },
        std::pair { GpuImageLayout::COLOR_ATTACHMENT, vk::ImageLayout::eColorAttachmentOptimal },
        std::pair { GpuImageLayout::DEPTH_STENCIL_ATTACHMENT, vk::ImageLayout::eDepthStencilAttachmentOptimal },
        std::pair { GpuImageLayout::READ_ONLY, vk::ImageLayout::eReadOnlyOptimalKHR },
        std::pair { GpuImageLayout::SHADER_READ_ONLY, vk::ImageLayout::eShaderReadOnlyOptimal },
        std::pair { GpuImageLayout::PREINITIALIZED, vk::ImageLayout::ePreinitialized },
    };
};
using Vulkan_GpuImageLayout = EnumAcceptor<GpuImageLayout, vk::ImageLayout>;

template <>
struct EnumMapping<GpuPipelineStage, vk::PipelineStageFlags2KHR>
{
    constexpr static auto MAPPING = {
        std::pair { GpuPipelineStage::NONE, vk::PipelineStageFlagBits2KHR::eNone },
        std::pair { GpuPipelineStage::VERTEX_INPUT, vk::PipelineStageFlagBits2KHR::eVertexInput },
        std::pair { GpuPipelineStage::VERTEX_SHADER, vk::PipelineStageFlagBits2KHR::eVertexShader },
        std::pair { GpuPipelineStage::GEOMETRY_SHADER, vk::PipelineStageFlagBits2KHR::eGeometryShader },
        std::pair { GpuPipelineStage::FRAGMENT_SHADER, vk::PipelineStageFlagBits2KHR::eFragmentShader },
        std::pair { GpuPipelineStage::EARLY_FRAGMENT_TESTS, vk::PipelineStageFlagBits2KHR::eEarlyFragmentTests },
        std::pair { GpuPipelineStage::LATE_FRAGMENT_TESTS, vk::PipelineStageFlagBits2KHR::eLateFragmentTests },
        std::pair { GpuPipelineStage::COLOR_ATTACHMENT_OUTPUT, vk::PipelineStageFlagBits2KHR::eColorAttachmentOutput },
        std::pair { GpuPipelineStage::TRANSFER, vk::PipelineStageFlagBits2KHR::eTransfer },
        std::pair { GpuPipelineStage::TRANSFER_COPY, vk::PipelineStageFlagBits2KHR::eCopy },
        std::pair { GpuPipelineStage::TRANSFER_RESOLVE, vk::PipelineStageFlagBits2KHR::eResolve },
        std::pair { GpuPipelineStage::TRANSFER_BLIT, vk::PipelineStageFlagBits2KHR::eBlit },
        std::pair { GpuPipelineStage::TRANSFER_CLEAR, vk::PipelineStageFlagBits2KHR::eClear },
        std::pair { GpuPipelineStage::ALL_COMMANDS, vk::PipelineStageFlagBits2KHR::eAllCommands },
        std::pair { GpuPipelineStage::HOST, vk::PipelineStageFlagBits2KHR::eHost },
        std::pair { GpuPipelineStage::TOP_OF_PIPE, vk::PipelineStageFlagBits2KHR::eTopOfPipe },
        std::pair { GpuPipelineStage::BOTTOM_OF_PIPE, vk::PipelineStageFlagBits2KHR::eBottomOfPipe },
    };
};
using Vulkan_GpuPipelineStage = EnumAcceptor<GpuPipelineStage, vk::PipelineStageFlags2KHR>;

template <>
struct EnumMapping<GpuAccessMask, vk::AccessFlags2KHR>
{
    constexpr static auto MAPPING = {
        std::pair { GpuAccessMask::NONE, vk::AccessFlagBits2KHR::eNone },
        std::pair { GpuAccessMask::HOST_READ, vk::AccessFlagBits2KHR::eHostRead },
        std::pair { GpuAccessMask::HOST_WRITE, vk::AccessFlagBits2KHR::eHostWrite },
        std::pair { GpuAccessMask::TRANSFER_READ, vk::AccessFlagBits2KHR::eTransferRead },
        std::pair { GpuAccessMask::TRANSFER_WRITE, vk::AccessFlagBits2KHR::eTransferWrite },
        std::pair { GpuAccessMask::MEMORY_READ, vk::AccessFlagBits2KHR::eMemoryRead },
        std::pair { GpuAccessMask::MEMORY_WRITE, vk::AccessFlagBits2KHR::eMemoryWrite },
        std::pair { GpuAccessMask::VERTEX_ATTRIBUTE_READ, vk::AccessFlagBits2KHR::eVertexAttributeRead },
        std::pair { GpuAccessMask::INDEX_READ, vk::AccessFlagBits2KHR::eIndexRead },
        std::pair { GpuAccessMask::UNIFORM_READ, vk::AccessFlagBits2KHR::eUniformRead },
        std::pair { GpuAccessMask::INPUT_ATTACHMENT_READ, vk::AccessFlagBits2KHR::eInputAttachmentRead },
        std::pair { GpuAccessMask::SHADER_READ, vk::AccessFlagBits2KHR::eShaderRead },
        std::pair { GpuAccessMask::SHADER_WRITE, vk::AccessFlagBits2KHR::eShaderWrite },
        std::pair { GpuAccessMask::COLOR_ATTACHMENT_READ, vk::AccessFlagBits2KHR::eColorAttachmentRead },
        std::pair { GpuAccessMask::COLOR_ATTACHMENT_WRITE, vk::AccessFlagBits2KHR::eColorAttachmentWrite },
        std::pair { GpuAccessMask::DEPTH_STENCIL_ATTACHMENT_READ, vk::AccessFlagBits2KHR::eDepthStencilAttachmentRead },
        std::pair { GpuAccessMask::DEPTH_STENCIL_ATTACHMENT_WRITE, vk::AccessFlagBits2KHR::eDepthStencilAttachmentWrite },
    };
};
using Vulkan_GpuAccessMask = EnumAcceptor<GpuAccessMask, vk::AccessFlags2KHR>;

template <>
struct EnumMapping<GpuVertexInputRate, vk::VertexInputRate>
{
    constexpr static auto MAPPING = {
        std::pair { GpuVertexInputRate::VERTEX, vk::VertexInputRate::eVertex },
        std::pair { GpuVertexInputRate::INSTANCE, vk::VertexInputRate::eInstance },
    };
};
using Vulkan_GpuVertexInputRate = EnumAcceptor<GpuVertexInputRate, vk::VertexInputRate>;

template <>
struct EnumMapping<GpuShaderStage, vk::ShaderStageFlagBits>
{
    constexpr static auto MAPPING = {
        std::pair { GpuShaderStage::VERTEX, vk::ShaderStageFlagBits::eVertex },
        std::pair { GpuShaderStage::FRAGMENT, vk::ShaderStageFlagBits::eFragment },
    };
};
using Vulkan_GpuShaderStage = EnumAcceptor<GpuShaderStage, vk::ShaderStageFlagBits>;

template <>
struct EnumMapping<GpuBufferFormat, vk::Format>
{
    constexpr static auto MAPPING = {
        std::pair { GpuBufferFormat::R8_UNORM, vk::Format::eR8Unorm },
        std::pair { GpuBufferFormat::R8G8_UNORM, vk::Format::eR8G8Unorm },
        std::pair { GpuBufferFormat::R8G8B8_UNORM, vk::Format::eR8G8B8Unorm },
        std::pair { GpuBufferFormat::R8G8B8A8_UNORM, vk::Format::eR8G8B8A8Unorm },
        std::pair { GpuBufferFormat::B8G8R8A8_UNORM, vk::Format::eB8G8R8A8Unorm },
        std::pair { GpuBufferFormat::R32_SFLOAT, vk::Format::eR32Sfloat },
        std::pair { GpuBufferFormat::R32G32_SFLOAT, vk::Format::eR32G32Sfloat },
        std::pair { GpuBufferFormat::R32G32B32_SFLOAT, vk::Format::eR32G32B32Sfloat },
        std::pair { GpuBufferFormat::R32G32B32A32_SFLOAT, vk::Format::eR32G32B32A32Sfloat },
        std::pair { GpuBufferFormat::D16_UNORM, vk::Format::eD16Unorm },
        std::pair { GpuBufferFormat::D32_SFLOAT, vk::Format::eD32Sfloat },
        std::pair { GpuBufferFormat::D16_UNORM_S8_UINT, vk::Format::eD16UnormS8Uint },
        std::pair { GpuBufferFormat::D24_UNORM_S8_UINT, vk::Format::eD24UnormS8Uint },
        std::pair { GpuBufferFormat::D32_SFLOAT_S8_UINT, vk::Format::eD32SfloatS8Uint },
    };
};
using Vulkan_GpuBufferFormat = EnumAcceptor<GpuBufferFormat, vk::Format>;

template <>
struct EnumMapping<GpuBufferUsage, vk::BufferUsageFlagBits>
{
    constexpr static auto MAPPING = {
        std::pair { GpuBufferUsage::VERTEX, vk::BufferUsageFlagBits::eVertexBuffer },
        std::pair { GpuBufferUsage::INDEX, vk::BufferUsageFlagBits::eIndexBuffer },
        std::pair { GpuBufferUsage::UNIFORM, vk::BufferUsageFlagBits::eUniformBuffer },
    };
};
using Vulkan_GpuBufferUsage = EnumAcceptor<GpuBufferUsage, vk::BufferUsageFlags>;
}
