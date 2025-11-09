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

### 7.1. Generators and Coroutines

Generators are functions that can be suspended and resumed. Coroutines (threads) are similar but have their own execution stack.

```quirrel
function count_to(n) {
    for (local i = 1; i <= n; i++) {
        yield i;
    }
}

let counter = count_to(3);
local value;
while(value = resume counter) {
    println(value);
}
```

### 7.2. Destructuring Assignment

Unpack values from arrays or tables into variables.

```quirrel
let [a, b] = [1, 2];
let {x, y} = { x = 10, y = 20 };
```

### 7.3. Modules

Quirrel supports modules for code organization.

```quirrel
// file: my_module.nut
let PI = 3.14159;
function circle_area(radius) {
    return PI * radius * radius;
}

// file: main.nut
from "my_module.nut" import PI, circle_area;

println(circle_area(10));
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