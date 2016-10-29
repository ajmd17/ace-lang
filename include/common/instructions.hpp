#ifndef INSTRUCTIONS_HPP
#define INSTRUCTIONS_HPP

// arguments should be placed in the format:
// dest, src

// parameters are marked in square brackets
// i8  = 8-bit integer (1 byte)
// i16 = 16-bit integer (2 bytes)
// i32 = 32-bit integer (4 bytes)
// i64 = 64-bit integer (8 bytes)
// u8  = 8-bit unsigned integer (1 byte)
// u16 = 16-bit unsigned integer (2 bytes)
// u32 = 32-bit unsigned integer (4 bytes)
// u64 = 64-bit unsigned integer (8 bytes)
// f32 = 32-bit float (4 bytes)
// f64 = 64-bit float (8 bytes)
// []  = array
// $   = stack offset (2 bytes)
// #   = static object index (2 bytes)
// %   = register (1 byte)
// @   = address (4 bytes)

enum Instructions : char {
    /* No operation */
    NOP = 0x00, // nop

    /* Store values in static memory */
    STORE_STATIC_STRING,   // str  [u32 len, i8[] str]
    STORE_STATIC_ADDRESS,  // addr [@ addr]
    STORE_STATIC_FUNCTION, // func [@ addr, u8 nargs]
    STORE_STATIC_TYPE,     // type [u8 size]

    /* Load a value into a register */
    LOAD_I32,    // load_i32    [% reg, i32 val]
    LOAD_I64,    // load_i64    [% reg, i64 val]
    LOAD_F32,    // load_f32    [% reg, f32 val]
    LOAD_F64,    // load_f64    [% reg, f64 val]
    LOAD_LOCAL,  // load_local  [% reg, $ idx]
    LOAD_STATIC, // load_static [% reg, # idx]
    LOAD_MEM,    // load_mem    [% reg, % src, u8 idx]
    LOAD_NULL,   // load_null   [% reg]
    LOAD_TRUE,   // load_true   [% reg]
    LOAD_FALSE,  // load_false  [% reg]

    /* Copy register value to stack location */
    MOV,     // mov     [$ dst, % src]
    /* Copy register value to object member */
    MOV_MEM, // mov_mem [% dst_obj, u8 dst_idx, % src]
    /* Push a value from register to the stack */
    PUSH, // push [% src]
    POP,  // pop

    ECHO,         // echo [% reg]
    ECHO_NEWLINE, // echo_nl

    /* Jump to address stored in register */
    JMP, // jmp [% address]
    JE,  // je [% address]
    JNE, // jne [% address]
    JG,  // jg [% address]
    JGE, // jge [% address]

    CALL, // call [% function, u8 argc]
    RET,  // ret

    BEGIN_TRY, // begin_try [% catch_addr]
    END_TRY,

    NEW, // new [% dst, # type_idx]

    /* Compare to register values */
    CMP,  // cmp [% lhs, % rhs]
    CMPZ, // cmpz [% lhs]

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
