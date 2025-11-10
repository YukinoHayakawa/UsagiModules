// entity.nut
// Represents a single entity's logic

from "engine_core" import/*  native_log, */ get_delta_time, GameObject

class Entity {
    cpp_obj = null
    state = null // This will hold our persistent state

    constructor(entity_id, entity_name) {
        print($"entity.nut: constructor for {entity_name}")

        // Create a C++ GameObject instance
        this.cpp_obj = GameObject(entity_id)

        // *** HOT-RELOAD STATE ***
        // Use 'persist' to get/create our state.
        // The state will survive script reloads.
        local state_key = $"entity_state_{entity_id}"
        this.state = persist(state_key, @() {
            // native_log($"entity.nut: Initializing NEW persistent state for {state_key}")
            // return {
                name = entity_name,
                tick_count = 0,
                x_pos = 0.0
            // }
        })

        // On reload, this log will show the old tick_count
        print($"entity.nut: {this.state.name} loaded with tick_count = {this.state.tick_count}")
    }

    // This is the coroutine function
    function UpdateCoroutine() {
        print($"{this.state.name} UpdateCoroutine started.")
        suspend("<", "xxx", ">")

        // This is the main game loop for this entity
        while (true) {
            // --- 1. Update State ---
            this.state.tick_count++
            this.state.x_pos += get_delta_time() * 1.0 // Move 1 unit/sec

            // Call C++ method
            this.cpp_obj.SetPosition(this.state.x_pos, 0.0, 0.0)

            // --- 2. Yield a Command ---
            // On tick 100, 200, etc., yield a command to C++
            if (this.state.tick_count != 0) {
                let cmd = $"CHECKPOINT_{this.state.tick_count}"
                suspend("<",cmd,">") // Pauses and sends string to C++
                //suspend("CHECKPOINT") // Pauses and sends string to C++
                //native_log($"{this.state.name} yielding command: {cmd}")
            }

            // --- 3. Wait for next frame ---
            // 'yield null' (or just 'yield') pauses execution
            // until C++ resumes us on the next tick.
            suspend(null)
        }
    }
}

// !!! YOU MUST EXPORT THE CLASS SO IT CAN BE IMPORTED BY ANOTHER SCRIPT !!!
return {
    Entity = Entity
}
