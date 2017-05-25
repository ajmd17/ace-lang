## Built-in Libraries

#### _Global_
The `_Global_` module contains everything that is usable without having to write the module name.

Functions:
 - `prompt(message: String) -> String`: Prompts the user with a message in the command line and returns the line they entered in a string
 - `to_string(object: Any) -> String`: Converts an object of any type to a string representation
 - `to_json(object: Any) -> String`: Converts an object of any type to a JSON representation held in a string
 - `decompile(f: Function) -> String`: Extracts the bytecode representation of a function object into a string
 - `fmt(format: String, args: Any...) -> String`: Uses a format specifier string and an arbitrary number of arguments of any type to build a new string, replacing '%' characters in the format string with passed arguments. Similar to `printf` in C
 - `array_push(arr: Array, args: Any...) -> Array`: Modifies a given array by appending every other passed argument to it
 - `length(object: Array | String) -> Int`: Returns the length of an array or string object
 - `call(f: Function, args: Any...) -> Any`: Passes the supplied arguments to the given function, and calls it. The return value will be the return value of the supplied function.
 - `spawn_thread(f: Function, args: Any...) -> Any`: Functions the same way that `call` does, however the function will be called on new thread and run concurrently

#### `runtime` Library
  - `gc() -> Null`: Triggers the runtime's garbage collector. Does not return a value.
  - `dump_heap() -> Null`: Prints out an information table of all objects stored in heap memory. Does not return a value.
  - `dump_stack() -> Null`: Prints out an information table of all objects stored on the stack (local variables). Does not return a value.
  - `typeof(object: Any) -> String`: Returns the run-time type of any object as a string.
  - `load_library(path: String) -> Any`: Loads a native dynamically linked library file (.dll, .dylib, .so) and returns a native pointer to the loaded library
  - `load_function(lib: Any, function_name: String) -> Function`: Uses a native pointer to a library (returned from `load_library`) and finds a function with the given name. Returns the native function object.
  - `version: Int[]`: An array with 3 elements, representing the Ace runtime version (major, minor, patch)
  - `os_name: String`: The name of the host operating system in a string (Mac, Linux, Windows)