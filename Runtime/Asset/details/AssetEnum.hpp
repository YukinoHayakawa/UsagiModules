﻿#pragma once

#include <string_view>

namespace usagi
{
enum class AssetPriority : std::uint8_t
{
    // Scripts, collision models, etc. Frame data computation cannot be properly
    // done without presence of them.
    GAMEPLAY_CRITICAL   = 3,

    // Normal streamed assets such as graphics content, detailed textures,
    // fine models, etc. Missing them doesn't affect the computational
    // correctness of data, but will be disturbing.
    LOD_STREAMING       = 2,

    // BGM, surrounding details, etc. Doesn't affect gameplay and are generally
    // unnoticeable when loaded slowly, unlike blurry textures or missing
    // model details.
    ENV_AMBIENT         = 1,
};

std::string_view to_string(AssetPriority val);

// todo update desc
enum class AssetStatus : std::uint8_t
{
    // The asset could not be found in any package.
    MISSING     = 0,

    // The asset exists, but the current operation will not cause it to be
    // loaded into memory.
    EXIST       = 1,

    // The asset exists but the file is locked by external program and
    // currently cannot be accessed.
    EXIST_BUSY  = 2,

    // A task has been queued to load the asset into memory.
    QUEUED      = 3,

    // A task is actively loading the content of asset into memory.
    LOADING     = 4,

    // The asset is loaded.
    READY       = 5,

    // The asset was failed to load.
    FAILED      = 6,

    // The asset is usable, but its content may be outdated due to changes
    // made to dependent assets or asset packages.
    OUTDATED    = 7,

    // A primary dependency could not be found.
    // Although missing a dependency could be a critical error, it is not hard
    // to fix once detected. A status flag provides an opportunity for the
    // caller to decide how to react to the incident. On the other hand,
    // the failure of secondary asset handler in processing the asset usually
    // indicates more serious errors such as bugs. So in that case, it's
    // more suitable to throw an exception.
    MISSING_DEPENDENCY = 8,
};

std::string_view to_string(AssetStatus val);
}
