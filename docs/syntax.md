## Syntax

### Statements

Statements in Ace do not require semicolons. 
Don't use them unless you have to keep multiple statements on the same line (in which case, you should consider changing the code to be clearer anyway).

#### Hello World

We can print any value to the console by using the built-in `print` statement, like this:

```
print 'Hello, world!'
```

Notice that we placed `'Hello, world!'` in single quotes. 
In Ace, `String` objects can be typed with either single quotes or double quotes.

The print statement can also take multiple arguments, using commas:

```
print 'Hello ', 'World'
```

#### Variables

Variables can be declared within a module, function, or any other block of code.
Examples:
* Assuming the variable `x` has not been declared, `x = 5` will create a variable named `x` with a value of `5`. The type of `x` will be `Int`, which cannot be changed to a different type once it is created.

* You can explicitly state a type when declaring a variable. If you would want `x` to be compatible with both the `Int` type as well as the `Float` type, you could declare `x` using this syntax: `x: Number = 5`.

* Similarly, `x: Any = 5` will declare a variable named `x` with the value `5`, however `x` will be assignable to any possible type afterwords, meaning that the code: `x = "Hello"` is perfectly valid.

* If you wish to create a new variable with a name that already exists in a parent scope, you may use the `let` keyword to explicitly tell Ace to create a new variable. This will not overwrite the already existing variable, but it will shadowed by the new variable and thus inaccessible until you are back in the parent scope.

When you create a variable, it will be set to the type of whatever you assign it to. If there is no type provided, you must manually enter the type you want it to be after the `:` character. To let a variable be able to be fluid between types, you can manually tell it to be `Any`.

Now that that's out of the way, let's see some example code!

```
b: Int       // creates 'b' with type 'Int' (will be set to 0 by default)

a = 5        // creates 'a' with type 'Int' and sets it to be 5
b = 10       // sets 'b' which was already declared to 10

print a      // prints out the number 5
print a + b  // prints out the number 15

// a = "Hello"    ERROR: cannot change the type from 'Int' to 'String'
c: Any = 100  // creates 'c' and sets it to be 100

print c       // prints out the number 100

c = "Hello, again"

print c       // prints 'Hello, again' onto the screen
```

#### Basic Types

Ace contains several built-in types you can use. Each type is either a _value_ type or a _reference_ type.

The difference between the two is that when a variable is passed to a function call, used to assign another variable, or copied in any other way:
- Value types are copied as they are much simpler and smaller objects

- Copying a reference type will cause both copies to refer to the _same_ value. This makes it more efficient to handle larger structures that are able to dynamically resize.

Built-in _value_ types include: `Number` (includes `Int` and `Float`), `Boolean`.

Built-in _reference_ types include: `Function`, `String`, `Array`, or any `Object` (which includes custom `type` definitions).

Example (showing the differences between value types and reference types):
```
/* ===== value types ===== */

i: Int = 10
j: Int = i

j += 10

// j is 20, but i is still 10.

/* ===== reference types ===== */

array1: Array = ['a', 'b', 'c']
array2: Array = array1

array2[0] = 'z'

// both array1 and array 2 are ['z', 'b', 'c'] 

```

#### Functions

General explanation on functions: http://www.webopedia.com/TERM/F/function.html

Functions are written using a pair of parentheses which may optionally contain parameters to be passed when the function is called. After the parentheses, the function's statements are written within a set of curly braces.

In Ace, functions are values themseleves. A variable may be assigned to a function in the same way that a variable may be assigned to a number or string.

Example:

```
return_2_plus_2 = () {
    return 2 + 2
}

print return_2_plus_2() // prints out 4
```

Example (with arguments)
```
return_a_plus_2 = (a: Number) {
    return a + 2
}

print return_a_plus_2(10) // prints 12
print return_a_plus_2(16) // prints 18
```

#### Modules

In Ace, each code file represents a individual module. Modules allow code to be pulled in and used from other files. Ace automatically creates a module named after the file it is contained in. 

Within any module, you may also created nested modules to keep things organized.

Example:
```
// in file my_program.ace

module hidden_details {
    // statements can in here
}
```

Ace automatically converts this to the form:
```
module my_program {
    // in file my_program.ace

    module hidden_details {
        // statements can in here
    }
}
```

#### Importing modules

In order to use code from another `module`, use the `import` keyword, with the module name directly after it.
Assuming you are currently writing in a separate file and want to import a file called `ace_example.ace`, you would use this syntax:
```
import ace_example
```

To import a nested module within the `ace_example` module (such as the `hidden_details` module from above), you
would use this syntax:
```
import ace_example::hidden_details
```

This will allow you to only write out `hidden_details` rather than writing out `ace_example::hidden_details` anytime you wish to use something from that module.

You can also import multiple nested modules at the same time:
```
import ace_example::{hidden_details, some_other_module}
```

If the file you wish to import is located in a separate folder than the current file, you may add a library search path using the `use library` syntax.

Assuming you want to be able to import modules from a directory above the current directory, you would use this syntax:
```
use library ['../']
```

Because the library search paths use an array, this means you can add multiple search paths at once.
Example
```
use library ['../', '../acelib', './some_library_folder']
```

#### Custom Types

Similar to many other languages, you can create your own custom, reusable types, which may contain nested objects (also called fields).

All custom types are _reference_ types, meaning any copies will point to the original value and modifying a copy will also modify any other copies.

To create a new type, use the `type` keyword.
Example:

```
type Person {
    id: Int
    name: String = "default value"
}

person: Person
person.name = "Banana Man"

print person.name // prints out: Banana Man

```

#### Methods
In Ace, all functions are held in variables, the same way that a number of string is. Methods are a fancy way of saying "fields that are functions". When you are defining your own types, you may choose to add functions that can be used to implement functionality centered around an object.

Expanding on our previous example above:
```
type Person {
    id: Int
    name: String = "default value"
    
    // method to make a person say hello
    say_hello = (self) {
        print fmt("% says hello!", self.name)
    }

    // methods can omit the '=' part to be more concise:
    say_something(self, what_to_say) {
        print fmt("% says: '%'", self.name, what_to_say)
    }
}

person: Person
person.name = "Banana Man"
person.say_hello() // Banana Man says hello!
//              ^ Note that the 'person' variable gets passed in as the 'self' argument.

person.say_something("I'm giving up on you") // Banana Man says: 'I'm giving up on you'
```

#### Actions

Actions provide an easy way to inform an object of something or provide it with data. Actions can be chained together to "pipe" data through a filter.

In contrast to methods, it is perfectly valid for an object to not have a particular actions handler implemented, whereas calling a method which does not exist on an object will cause an error.

Actions are added to a `type` using the `on` syntax, followed by any value (key) the action should be equal to, a fat arrow (`=>`), and a handler function.

Example:

```
type Player {
    // ...

    on 'move left' => (self, amount: Number) {
        self.x -= amount
    }
}

player: Player

/* first item in the list will
   be the key: what the 'on' should check for */

'move left', 2.5 => player
```

The functionality of actions is still being worked out.

#### Generators

Generators are functions which provide an easy way to iterate through a sequence. They allow the use of the `yield` keyword, which causes the current function to be temporarily stepped out of to perform another action, and then continue where it left off before.

Generators are created using a `*` symbol before the `{` when creating a function.

Example (defining a generator function):
```
loop: Function = (from: Number, to: Number) * {
    i: Number = from

    while i < to {
        // temporarily jump out
        yield i

        // continue where we left off
        i += 1
    }
}
```

To use a generator, use the `action` symbol (`=>`), followed by a function.

Usage:
```
loop(10, 20) => (i: Number) {
    print "i = ", i
}

// prints out:
// 10
// 11
// 12
// ...more numbers...
// 19

```