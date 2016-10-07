#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

// arguments should be placed in the format:
// dest, src

// parameters are marked in square brackets
// i8 = 8-bit integer (1 byte)
// i32 = 32-bit integer (4 bytes)
// i64 = 64-bit integer (8 bytes)
// u8 = 8-bit unsigned integer (1 byte)
// u32 = 32-bit unsigned integer (4 bytes)
// u64 = 64-bit unsigned integer (8 bytes)
// f = float (4 bytes)
// d = double (8 bytes)
// $ = stack offset (2 bytes)
// % = register (1 byte)
// @ = address (4 bytes)

enum Instructions : char {
    /* Load a value into a register */
    LOAD_I32,  // load_i32  [% reg, i32 val]
    LOAD_I64,  // load_i64  [% reg, i64 val]
    LOAD_F,    // load_f    [% reg, f val]
    LOAD_D,    // load_d    [% reg, d val]
    LOAD_ADDR, // load_addr [% reg, i32 val]
    LOAD_LOCAL, // load_local [% reg, $ idx]

    /* Push a value from register to the stack */
    PUSH,     // push     [% src]

    POP, // pop

    /* Jump to address stored in register */
    JMP, // jmp [% address]
    JE, // je [% address]
    JNE, // jne [% address]
    JG, // jg [% address]
    JGE, // jge [% address]

    CALL, // call [% function, u8 argc]
    RET, // ret

    /* Compare to register values */
    CMP, // cmp [% lhs, % rhs]

    /* Mathematical operations */
    ADD, // add [% lhs, % rhs, % dst]
    SUB,
    MUL,
    DIV,
    MOD,

    /* Signifies the end of the stream */
    EXIT,
};

#endif