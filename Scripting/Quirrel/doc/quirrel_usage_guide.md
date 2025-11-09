# Quirrel Language Usage Guide

## Introduction

Quirrel is a fast, high-level, imperative, object-oriented programming language, designed to be a lightweight but powerful scripting tool. It is a fork of the Squirrel language with a focus on safety, performance, and modern features. Its syntax is similar to C-like languages (C++, Java, C#) and it is easily embeddable in C++ applications.

This guide provides a comprehensive overview of the Quirrel language, with code examples to illustrate its features.

## 1. Lexical Structure

### 1.1. Comments

Quirrel supports C-style block comments (`/* ... */`) and C++-style single-line comments (`// ...`).

```quirrel
/*
 * This is a multi-line block comment.
 */
local a = 10; // This is a single-line comment.
```

### 1.2. Identifiers

Identifiers are names used to identify variables, functions, classes, etc. They must start with a letter or an underscore, followed by any number of letters, digits, or underscores. Quirrel is case-sensitive.

```quirrel
local my_variable = "Hello";
local _anotherVariable = 123;
```

### 1.3. Keywords

The following words are reserved and cannot be used as identifiers:

`base`, `break`, `case`, `catch`, `class`, `clone`, `continue`, `const`, `default`, `delete`, `else`, `enum`, `for`, `foreach`, `function`, `if`, `in`, `local`, `null`, `resume`, `return`, `switch`, `this`, `throw`, `try`, `typeof`, `while`, `yield`, `constructor`, `instanceof`, `true`, `false`, `static`, `__LINE__`, `__FILE__`, `let`

### 1.4. Literals

#### 1.4.1. Integer

Integers can be specified in decimal, hexadecimal, or as character codes. Underscores can be used as separators for readability.

```quirrel
local decimal = 12345;
local hex = 0xFF;
local with_separators = 1_000_000;
local char_code = 'A'; // 65
```

#### 1.4.2. Float

Floats are used for floating-point numbers.

```quirrel
local pi = 3.14;
local scientific = 1.2e-5;
```

#### 1.4.3. String

Strings are immutable sequences of characters. They can be enclosed in single or double quotes. Verbatim strings, prefixed with `@`, do not process escape sequences.

```quirrel
local greeting = "Hello, Quirrel!";
local path = @"C:\path\to\file";
local multiline = @"
This is a
multi-line string.
";
```

String interpolation is supported using the `$` prefix.

```quirrel
local name = "Yukino";
local message = $"Hello, {name}!"; // "Hello, Yukino!"
```

## 2. Data Types

Quirrel is a dynamically typed language. The basic types are: `integer`, `float`, `string`, `null`, `bool`, `table`, `array`, `function`, `class`, `instance`, `generator`, `thread`, and `userdata`.

### 2.1. Variables and Bindings

Variables can be declared using `local` (mutable) or `let` (immutable binding). `const` is used for compile-time constants.

```quirrel
local a = 10; // Can be reassigned
a = 20;

let b = 30; // Cannot be reassigned
// b = 40; // This would cause a compilation error

const C = 50; // Compile-time constant
```

### 2.2. Tables (Associative Arrays)

Tables are key-value stores, similar to dictionaries or hash maps in other languages.

```quirrel
local my_table = {
    name = "Shio",
    age = 1,
    is_agent = true
};

// Accessing members
println(my_table.name); // "Shio"
println(my_table["age"]); // 1

// Adding a new slot
my_table.new_slot <- "new value";
```

### 2.3. Arrays

Arrays are ordered sequences of values.

```quirrel
local my_array = [1, "two", { a = 3 }];
println(my_array[0]); // 1

my_array.append(4);
```

## 3. Expressions and Operators

Quirrel supports a wide range of operators.

### 3.1. Arithmetic Operators

`+`, `-`, `*`, `/`, `%` (modulo), `++`, `--`, `+=`, `-=`, `*=`, `/=`, `%=`.

### 3.2. Relational and Equality Operators

`==`, `!=`, `<`, `>`, `<=`, `>=`, `<=>` (three-way comparison).

### 3.3. Logical Operators

`&&` (and), `||` (or), `!` (not).

### 3.4. Bitwise Operators

`&`, `|`, `^`, `~`, `<<`, `>>`, `>>>` (unsigned right shift).

### 3.5. Null-Coalescing and Propagation

The null-coalescing operator `??` returns the left-hand operand if it's not null, otherwise the right-hand operand.
The null-propagation operators `?.` and `?[]` allow safe access to members of potentially null objects.

```quirrel
local t = null;
local value = t?.x ?? "default"; // value will be "default"
```

## 4. Control Structures

### 4.1. `if-else`

```quirrel
if (condition) {
    // ...
} else if (another_condition) {
    // ...
} else {
    // ...
}
```

### 4.2. Loops: `while`, `do-while`, `for`, `foreach`

```quirrel
// while
while (condition) {
    // ...
}

// do-while
do {
    // ...
} while (condition);

// for
for (local i = 0; i < 10; i++) {
    // ...
}

// foreach
foreach (key, value in my_table) {
    // ...
}
```

### 4.3. `switch` (Deprecated)

The `switch` statement is available but deprecated in favor of `if-else` chains.

### 4.4. `try-catch`

For exception handling.

```quirrel
try {
    throw "An error occurred";
} catch (e) {
    println($"Caught exception: {e}");
}
```

## 5. Functions

Functions are first-class citizens in Quirrel.

```quirrel
function add(a, b) {
    return a + b;
}

// Lambda expression
let multiply = @(a, b) a * b;

// Function with default parameters and varargs
function format(fmt, ...) {
    // ...
}
```

## 6. Classes and Inheritance

Quirrel supports classes and single inheritance.

```quirrel
class Vehicle {
    constructor(name) {
        this.name = name;
    }

    function move() {
        println($"{name} is moving.");
    }
}

class Car(Vehicle) {
    constructor(name, brand) {
        base.constructor(name); // Call base class constructor
        this.brand = brand;
    }

    function move() {
        base.move();
        println("On four wheels.");
    }
}

let my_car = Car("My Car", "Usagi");
my_car.move();
```

### 6.1. Static Members

Static members are shared among all instances of a class.

```quirrel
class Counter {
    static count = 0;
    constructor() {
        Counter.count++;
    }
}
```

### 6.2. Metamethods

Classes can define metamethods (like `_add`, `_get`, `_set`) to customize their behavior, similar to operator overloading.

## 7. Advanced Features

### 7.1. Concurrency: Generators and Coroutines

Quirrel provides two powerful mechanisms for managing control flow: generators for iteration and coroutines for concurrent tasks. While they both use `yield`, they serve different purposes.

#### 7.1.1. Generators

A generator is a function that can be suspended and resumed, producing a sequence of values on demand. They are essentially resumable functions that run on the **caller's stack**. Generators are excellent for creating custom, lazy iterators.

When a generator function finishes, its state becomes `"dead"`.

```quirrel
// This function is a generator
function count_to(n) {
    for (local i = 1; i <= n; i++) {
        yield i; // Pauses and returns the value of i
    }
}

let counter_gen = count_to(3);
println(counter_gen.getstatus()); // "suspended"

local value;
while(value = resume counter_gen) {
    println(value);
}
// Output:
// 1
// 2
// 3

println(counter_gen.getstatus()); // "dead"
```

#### 7.1.2. Coroutines (Threads)

A coroutine, called a `thread` in Quirrel, is a function that has its **own execution stack**. This allows it to be suspended and resumed independently of the main program flow, making it ideal for cooperative multitasking, such as managing an entity's logic over multiple game frames.

##### Coroutine Lifecycle and Status

A coroutine transitions through several states, which can be queried using the `getstatus()` method:

1.  **`"idle"`**: The initial state of a new coroutine, or the state after it has finished executing its function. A C++ host can check for this state to know a coroutine has completed its work.
2.  **`"running"`**: The coroutine is currently executing code.
3.  **`"suspended"`**: The coroutine is paused, having called `yield` or `suspend`. It is waiting to be resumed by a call to `wakeup()`.

```quirrel
let co = newthread(function(val) {
    println($"Coroutine started with: {val}");
    suspend(); // Pause and wait to be woken up
    println("Coroutine resumed.");
});

println(co.getstatus()); // "idle"

co.call("Hello"); // Start the coroutine
println(co.getstatus()); // "suspended"

co.wakeup(); // Resume the coroutine
println(co.getstatus()); // "idle" (it has now finished)
```

##### Bidirectional Communication with `suspend`

The global `suspend(...)` function is the primary mechanism for a coroutine to communicate with the C++ host. It is a variadic function that can both send multiple values to the host and receive a value back.

-   **Sending Data to Host**: Any arguments passed to `suspend` are pushed to the stack for the C++ host to read.
-   **Receiving Data from Host**: The value returned by `suspend` is the value the C++ host provides when it wakes the coroutine up.

```quirrel
// --- In Script ---
function RequestDataCoroutine(id) {
    // Send a command and an ID to the C++ host, and wait for a result.
    let result = suspend("GET_ENTITY_NAME", id);

    // When resumed, 'result' will hold the value from C++.
    println($"Received entity name: {result}");
    return result;
}

// --- In C++ (Conceptual) ---
// After sq_resume(v, ...) returns SQ_SUSPEND, the host can:
// 1. Check the stack for arguments passed to suspend().
//    e.g., read the string "GET_ENTITY_NAME" and the integer id.
// 2. Find the requested data (e.g., from a database).
// 3. Push the result (e.g., the entity's name string) onto the stack.
// 4. Call sq_wakeup(v, ...) to resume the coroutine. The pushed
//    value will become the return value of 'suspend' in the script.
```

##### Coroutine Object Methods

Thread objects have several built-in methods for managing them:

-   `my_thread.call(...)`: Starts the coroutine, passing arguments to its main function.
-   `my_thread.wakeup(value = null)`: Resumes a suspended coroutine. The optional `value` is passed as the return value of `yield` or `suspend`.
-   `my_thread.getstatus()`: Returns a string representing the current state: `"idle"`, `"running"`, or `"suspended"`.
-   `my_thread.getstackinfos(level)`: A powerful debugging tool that returns a table of information (function name, source, line number, locals) for a specific level of the coroutine's call stack.


### 7.2. Destructuring Assignment

Unpack values from arrays or tables into variables.

```quirrel
let [a, b] = [1, 2];
let {x, y} = { x = 10, y = 20 };
```

### 7.3. Modules and Hot-Reloading

Quirrel has a powerful module system for code organization, dependency management, and supporting live-script reloading (hot-reloading) without losing state.

#### 7.3.1. Exporting from a Module

A script file acts as a module. To export values, classes, or functions, a script should end with a `return` statement. The returned value is what importers will receive. It's idiomatic to return a table containing the public members of the module.

```quirrel
// file: vector.nut
class Vec2 {
  constructor(x, y) { this.x = x; this.y = y; }
  function length() { return Math.sqrt(x*x + y*y); }
}

function new_vec(x, y) {
  return Vec2(x, y);
}

// Export the class and the factory function
return {
  Vec2 = Vec2,
  new_vec = new_vec
}
```

#### 7.3.2. Importing with `require`

The `require(path)` function executes a script and returns the value from its `return` statement. This is the most common way to import a module. You can assign the entire module table to a variable or use destructuring to unpack specific members.

```quirrel
// Import the whole module table
local VectorModule = require("vector.nut");
let v1 = VectorModule.new_vec(3, 4);

// Use destructuring to import specific members
let { Vec2, new_vec } = require("vector.nut");
let v2 = Vec2(5, 12);
println(v2.length());
```

#### 7.3.3. Importing with `from ... import`

As an alternative to `require`, you can use the `from ... import` syntax, which may be more familiar to Python developers. It imports the specified members directly into the current scope.

```quirrel
// file: main.nut
from "vector.nut" import Vec2, new_vec;

let v = new_vec(1, 1);
```

#### 7.3.4. Module Hot-Reloading with `persist`

A key feature for game development is the ability to change script logic while the game is running. Quirrel facilitates this via the `persist(key, initializer)` global function. It allows state to survive script reloads.

-   `key`: A unique string to identify the persistent state.
-   `initializer`: A lambda function that is **only executed if the key is not found** in the persistent storage. It must return the initial value for the state.

On subsequent reloads, `persist` will return the already-existing value associated with the `key`, and the `initializer` lambda will be ignored.

**Example: Global State**

This preserves a global list of entities across reloads.

```quirrel
// game_logic.nut
let Entity = require("entity.nut");

// The 'g_state' table will survive reloads.
// The initializer runs only on the first load.
let g_state = persist("global_entity_list", @() {
    print("--- Initializing NEW global state ---");
    return {
        entities = [ Entity(1, "Player"), Entity(2, "Enemy") ]
    }
});
```

**Example: Instance State**

This pattern can be used within a class constructor to attach persistent state to an entity instance.

```quirrel
// entity.nut
class Entity {
    state = null // This will hold our persistent state

    constructor(entity_id) {
        // Use a unique key for this entity's state
        local state_key = $"entity_state_{entity_id}";

        // Get or create the persistent state for this instance
        this.state = persist(state_key, @() {
            print($"--- Initializing NEW state for {state_key} ---");
            return { tick_count = 0, position = 0.0 }
        })

        // This log will show the old tick_count on reload
        print($"Entity {entity_id} loaded with tick_count={this.state.tick_count}");
    }

    function update() {
        this.state.tick_count++;
    }
}
```


### 7.4. Compiler Directives

Directives like `#strict`, `#relaxed`, `#allow-clone-operator` can customize language features.

```quirrel
#strict // Enforce stricter rules

#forbid-delete-operator
// delete my_table.slot; // This would be a compile error
```

This guide covers the main features of the Quirrel language. For more in-depth information, please refer to the official documentation and the EBNF file.

```