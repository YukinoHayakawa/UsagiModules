#include "RbVulkanShaderModule.hpp"

#include <Usagi/Modules/Runtime/HeapManager/HeapManager.hpp>
#include <Usagi/Modules/Resources/ResGraphicsCommon/RbSpirvBytecodes.hpp>

namespace usagi
{
ResourceState RbVulkanShaderModule::construct(
    ResourceConstructDelegate<RbVulkanShaderModule> &delegate)
{
    // Get shader source code
    auto &bytecodes = delegate.resource<RbSpirvBytecodes>(
        arg<AssetPath>(),
        arg<GpuShaderStage>()
    ).await().bytecodes();

    // Create the object
    vk::ShaderModuleCreateInfo info;

    info.pCode = bytecodes.data();
    info.codeSize = bytecodes.size() * sizeof(std::uint32_t);

    delegate.allocate(info, bytecodes);

    return ResourceState::READY;
}
}
