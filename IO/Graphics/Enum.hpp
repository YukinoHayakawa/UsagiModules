#pragma once

namespace usagi
{
// Pipeline

enum class GpuPipelineStage
{
    NONE,
    VERTEX_INPUT,
    VERTEX_SHADER,
    GEOMETRY_SHADER,
    FRAGMENT_SHADER,
    EARLY_FRAGMENT_TESTS,
    LATE_FRAGMENT_TESTS,
    COLOR_ATTACHMENT_OUTPUT,
    TRANSFER,
    TRANSFER_COPY,
    TRANSFER_RESOLVE,
    TRANSFER_BLIT,
    TRANSFER_CLEAR,
    ALL_COMMANDS,
    TOP_OF_PIPE,
    BOTTOM_OF_PIPE,
    HOST,
};

enum class GpuAccessMask
{
    NONE,
    HOST_READ,
    HOST_WRITE,
    TRANSFER_READ,
    TRANSFER_WRITE,
    MEMORY_READ,
    MEMORY_WRITE,
    VERTEX_ATTRIBUTE_READ,
    INDEX_READ,
    UNIFORM_READ,
    INPUT_ATTACHMENT_READ,
    SHADER_READ,
    SHADER_WRITE,
    COLOR_ATTACHMENT_READ,
    COLOR_ATTACHMENT_WRITE,
    DEPTH_STENCIL_ATTACHMENT_READ,
    DEPTH_STENCIL_ATTACHMENT_WRITE,
};

// Images

enum class GpuImageLayout
{
    UNDEFINED,
    PRESENT,
    TRANSFER_SRC,
    TRANSFER_DST,
    ATTACHMENT,
    COLOR_ATTACHMENT,
    DEPTH_STENCIL_ATTACHMENT,
    READ_ONLY,
    SHADER_READ_ONLY,
    PREINITIALIZED,
    AUTO,
};

enum class GpuImageUsage
{
    SAMPLED,
    COLOR_ATTACHMENT,
    DEPTH_STENCIL_ATTACHMENT,
    INPUT_ATTACHMENT,
};

enum class GpuCompareOp
{
    NEVER,
    LESS,
    EQUAL,
    LESS_OR_EQUAL,
    GREATER,
    NOT_EQUAL,
    GREATER_OR_EQUAL,
    ALWAYS,
};

// Buffers

enum class GpuBufferFormat
{
    R8_UNORM,
    R8G8_UNORM,
    R8G8B8_UNORM,
    R8G8B8A8_UNORM,

    B8G8R8A8_UNORM,

    R32_SFLOAT,
    R32G32_SFLOAT,
    R32G32B32_SFLOAT,
    R32G32B32A32_SFLOAT,

    // Depth stencil formats

    D16_UNORM,
    D32_SFLOAT,
    D16_UNORM_S8_UINT,
    D24_UNORM_S8_UINT,
    D32_SFLOAT_S8_UINT,
};

enum class GpuBufferUsage
{
    VERTEX,
    INDEX,
    UNIFORM,
};

// Misc

enum class GpuAttachmentLoadOp
{
    LOAD,
    CLEAR,
    UNDEFINED,
};

enum class GpuAttachmentStoreOp
{
    STORE,
    UNDEFINED,
};

enum class GpuIndexType
{
    UINT16,
    UINT32,
};
}
