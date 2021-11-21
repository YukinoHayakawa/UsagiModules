#include "SahShaderModule.hpp"

#include <Usagi/Modules/Platforms/Vulkan/VulkanGpuDevice.hpp>

#include "SahGlslCompiler.hpp"

namespace usagi
{
SahShaderModule::SahShaderModule(
    VulkanGpuDevice *device,
    std::string asset_path,
    std::string stage): mDevice(device)
    , mAssetPath(std::move(asset_path))
    , mStage(std::move(stage))
{
    assert(mDevice);
}

std::unique_ptr<SecondaryAsset> SahShaderModule::construct()
{
	auto &bytecodes = await_secondary<SahGlslCompiler>(mAssetPath, mStage);

	vk::ShaderModuleCreateInfo info;
	info.pCode = bytecodes.data();
	info.codeSize = bytecodes.size() * sizeof(std::uint32_t);
	auto shader = mDevice->device().createShaderModuleUnique(
		info, nullptr, mDevice->dispatch());

    auto reflection_compiler = std::make_unique<spirv_cross::Compiler>(
        bytecodes);

    return std::make_unique<ShaderModule>(
        std::move(shader),
        std::move(reflection_compiler)
    );
}

void SahShaderModule::append_features(Hasher &hasher)
{
    // todo append device trait
    hasher.append(mAssetPath).append(mStage);
}
}
