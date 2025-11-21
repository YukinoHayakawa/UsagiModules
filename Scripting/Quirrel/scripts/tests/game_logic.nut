// game_logic.nut
// Main entry point for game logic

from "engine_core" import native_log, to_json_string

// seems local dosn't work and will trigger assert() failure
// ... actually it was due to `native_log`
// local Entity = require("entity.nut") // Use SqModules 'require'
let { Entity } = require("entity.nut") // Use SqModules 'require'

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

function test()
{
    print(to_json_string({
      "id": 1,
      "name": "Foo",
      "price": 123,
      "tags": ["Bar","Eek"],
      "a": 1,
      "b": 1,
      "c": 1,
      "d": 1,
      "e": 1,
    }));
    print(to_json_string({ entity = Entity(4, "Enemy_2") }));
    while(true)
    {
        suspend("get_delta_time()", 1, true)
    }
    //suspend("test")
    //return 1
}

// This is the factory function C++ will call
function GetAllEntityCoroutines() {
    native_log($"game_logic.nut: C++ requested 'GetAllEntityCoroutines'")
    local coroutines = []
    foreach(entity in g_state.entities) {
       // Add the 'UpdateCoroutine' function from each entity instance
        coroutines.append(entity.UpdateCoroutine.bindenv(entity))
    }
    coroutines.append(function() { while(true) { suspend("test") } })
    coroutines.append(test)
    return coroutines
}

// Export the factory function
return {
    GetAllEntityCoroutines = GetAllEntityCoroutines
}
