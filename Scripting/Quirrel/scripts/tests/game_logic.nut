// game_logic.nut
// Main entry point for game logic

from "engine_core" import native_log
local Entity = require("entity.nut") // Use SqModules 'require'

native_log("game_logic.nut: Top-level execution.")

// *** HOT-RELOAD STATE ***
// Use 'persist' to define our list of entities.
// This table will survive reloads.
// !!! Within the init lambda of persist, you can only define variables !!!
let g_state = persist("global_entity_list", @() {
    // native_log($"game_logic.nut: Initializing NEW global state.")
    // return {
        entities = [
            Entity(1, "Player"),
            Entity(2, "Enemy_1"),
            Entity(3, "Enemy_2")
        ]
    // }
})

native_log($"game_logic.nut: {g_state.entities.len()} entities loaded from persistent state.")

// This is the factory function C++ will call
function GetAllEntityCoroutines() {
    native_log($"game_logic.nut: C++ requested 'GetAllEntityCoroutines'")
    local coroutines = []
    foreach(entity in g_state.entities) {
        // Add the 'UpdateCoroutine' function from each entity instance
        coroutines.push(entity.UpdateCoroutine.bindenv(entity))
    }
    return coroutines
}

// Export the factory function
return {
    GetAllEntityCoroutines = GetAllEntityCoroutines
}
