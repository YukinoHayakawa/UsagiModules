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

class EntityWithoutPersist {
    x = 0
    y = 0
    z = 0

    constructor(x_, y_, z_) {
        this.x = x_
        this.y = y_
        this.z = z_
    }
}

class Nothing { }

native_log($"game_logic.nut: {g_state.entities.len()} entities loaded from persistent state.")

function test()
{
    // Seems that two anonymous tables would crash the vm during
    // >	QuirrelScripting.exe!SQTable::_GetStr(const unsigned __int64 key, unsigned __int64 hash) Line 89	C++
    // Becuase the second table is not allocated. It's weird.
    native_log(to_json_string({
      "id": 1,
      "name": "Foo",
      "price": 123,
      "tags": ["Bar","Eek"],
      "a": 1,
      "b": 1,
      "c": 1,
      "d": 1,
      "e": 1,
    }))
    // It seems that only having an instance like `Entity(4, "Enemy_2")` would
    // cause the crash. Could it because hot-reloading?
    // let temp = { "entity": Entity(4, "Enemy_2") }
    // A simple table like this never crashes.
    // let temp = { "entity": true }
    // No persist values. Still crashes.
    // let temp = { "entity": EntityWithoutPersist(1, 2, 3) }
    // Seems that an empty class doesn't crash the vm.
    // Ok. I am sure that the crashes are due to bugs in our print() impl.
    let temp = { "entity": Nothing() }
    let str = to_json_string(temp)
    native_log(str)
    native_log(to_json_string({ "entity": Entity(4, "Enemy_2") }))
    native_log(to_json_string({ "entity": Entity(4, "Enemy_2") }))
    native_log(to_json_string({ "entity": Entity(4, "Enemy_2") }))
    native_log(to_json_string({ "entity": Entity(4, "Enemy_2") }))
    native_log(to_json_string({ "entity": Entity(4, "Enemy_2") }))
    print(to_json_string({ "entity": Entity(4, "Enemy_2") }))
    print(to_json_string({ "entity": Entity(4, "Enemy_2") }))
    print(to_json_string({ "entity": Entity(4, "Enemy_2") }))
    print(to_json_string({ "entity": Entity(4, "Enemy_2") }))
    print(to_json_string({ "entity": Entity(4, "Enemy_2") }))
    print({ "entity": Entity(4, "Enemy_2") }.tostring())
    print({ "entity": Entity(4, "Enemy_2") }.tostring())
    print({ "entity": Entity(4, "Enemy_2") }.tostring())
    print({ "entity": Entity(4, "Enemy_2") }.tostring())
    print({ "entity": Entity(4, "Enemy_2") }.tostring())
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
    native_log(to_json_string({ "entity": Entity(4, "Enemy_2") }))
    native_log(to_json_string({ "entity": Entity(4, "Enemy_2") }))
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
