#pragma once

#include <utility>

#include <vulkan/vulkan.hpp>

#include <Usagi/Library/Utility/EnumTranslation.hpp>
#include <Usagi/Modules/Services/Graphics/Enum.hpp>

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
}
