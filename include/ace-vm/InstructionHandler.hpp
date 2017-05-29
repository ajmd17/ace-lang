#ifndef INSTRUCTION_HANDLER_HPP
#define INSTRUCTION_HANDLER_HPP

#include <ace-vm/BytecodeStream.hpp>
#include <ace-vm/VM.hpp>
#include <ace-vm/Value.hpp>
#include <ace-vm/HeapValue.hpp>
#include <ace-vm/Array.hpp>
#include <ace-vm/Object.hpp>
#include <ace-vm/TypeInfo.hpp>

#include <common/instructions.hpp>
#include <common/utf8.hpp>
#include <common/my_assert.hpp>
#include <common/typedefs.hpp>

#include <stdint.h>

namespace ace {
namespace vm {

struct InstructionHandler {
    VMState *state;
    ExecutionThread *thread;
    BytecodeStream *bs;

    InstructionHandler(VMState *state,
      ExecutionThread *thread,
      BytecodeStream *bs)
      : state(state),
        thread(thread),
        bs(bs)
    {
    }

    inline void StoreStaticString(uint32_t len, const char *str)
    {
        // the value will be freed on
        // the destructor call of state->m_static_memory
        HeapValue *hv = new HeapValue();
        hv->Assign(utf::Utf8String(str));

        Value sv;
        sv.m_type = Value::HEAP_POINTER;
        sv.m_value.ptr = hv;

        state->m_static_memory.Store(std::move(sv));
    }

    inline void StoreStaticAddress(uint32_t addr)
    {
        Value sv;
        sv.m_type = Value::ADDRESS;
        sv.m_value.addr = addr;

        state->m_static_memory.Store(std::move(sv));
    }

    inline void StoreStaticFunction(uint32_t addr,
        uint8_t nargs,
        uint8_t is_variadic)
    {
        Value sv;
        sv.m_type = Value::FUNCTION;
        sv.m_value.func.m_addr = addr;
        sv.m_value.func.m_nargs = nargs;
        sv.m_value.func.m_is_variadic = is_variadic;

        state->m_static_memory.Store(std::move(sv));
    }

    inline void StoreStaticType(const char *type_name,
        uint16_t size,
        char **names)
    {
        // the value will be freed on
        // the destructor call of state->m_static_memory
        HeapValue *hv = new HeapValue();
        hv->Assign(TypeInfo(type_name, size, names));

        Value sv;
        sv.m_type = Value::HEAP_POINTER;
        sv.m_value.ptr = hv;
        state->m_static_memory.Store(std::move(sv));
    }

    inline void LoadI32(uint8_t reg, aint32 i32)
    {
        // get register value given
        Value &value = thread->m_regs[reg];
        value.m_type = Value::I32;
        value.m_value.i32 = i32;
    }

    inline void LoadI64(uint8_t reg, aint64 i64)
    {
        // get register value given
        Value &value = thread->m_regs[reg];
        value.m_type = Value::I64;
        value.m_value.i64 = i64;
    }

    inline void LoadF32(uint8_t reg, afloat32 f32)
    {
        // get register value given
        Value &value = thread->m_regs[reg];
        value.m_type = Value::F32;
        value.m_value.f = f32;
    }

    inline void LoadF64(uint8_t reg, afloat64 f64)
    {
        // get register value given
        Value &value = thread->m_regs[reg];
        value.m_type = Value::F64;
        value.m_value.d = f64;
    }

    inline void LoadOffset(uint8_t reg, uint8_t offset)
    {
        // read value from stack at (sp - offset)
        // into the the register
        thread->m_regs[reg] = thread->m_stack[thread->m_stack.GetStackPointer() - offset];
    }

    inline void LoadIndex(uint8_t reg, uint16_t index)
    {
        // read value from stack at the index into the the register
        // NOTE: read from main thread
        thread->m_regs[reg] = state->MAIN_THREAD->m_stack[index];
    }

    inline void LoadStatic(uint8_t reg, uint16_t index)
    {
        // read value from static memory
        // at the index into the the register
        thread->m_regs[reg] = state->m_static_memory[index];
    }

    inline void LoadString(uint8_t reg, uint32_t len, const char *str)
    {
        // allocate heap value
        HeapValue *hv = state->HeapAlloc(thread);
        if (hv != nullptr) {
            hv->Assign(utf::Utf8String(str));

            // assign register value to the allocated object
            Value &sv = thread->m_regs[reg];
            sv.m_type = Value::HEAP_POINTER;
            sv.m_value.ptr = hv;
        }
    }

    inline void LoadAddr(uint8_t reg, uint8_t addr)
    {
        Value &sv = thread->m_regs[reg];
        sv.m_type = Value::ADDRESS;
        sv.m_value.addr = addr;
    }

    inline void LoadFunc(uint8_t reg,
        uint32_t addr,
        uint8_t nargs,
        uint8_t is_variadic)
    {
        Value &sv = thread->m_regs[reg];
        sv.m_type = Value::FUNCTION;
        sv.m_value.func.m_addr = addr;
        sv.m_value.func.m_nargs = nargs;
        sv.m_value.func.m_is_variadic = is_variadic;
    }

    inline void LoadType(uint8_t reg,
        uint16_t type_name_len,
        const char *type_name,
        uint16_t size,
        char **names)
    {
        // allocate heap value
        HeapValue *hv = state->HeapAlloc(thread);
        ASSERT(hv != nullptr);

        hv->Assign(TypeInfo(type_name, size, names));

        // assign register value to the allocated object
        Value &sv = thread->m_regs[reg];
        sv.m_type = Value::HEAP_POINTER;
        sv.m_value.ptr = hv;
    }

    inline void LoadMem(uint8_t dst, uint8_t src, uint8_t index)
    {
        Value &sv = thread->m_regs[src];
        ASSERT(sv.m_type == Value::HEAP_POINTER);

        HeapValue *hv = sv.m_value.ptr;
        if (hv == nullptr) {
            state->ThrowException(thread, Exception::NullReferenceException());
        } else {
            if (Object *obj_ptr = hv->GetPointer<Object>()) {
                const vm::TypeInfo *type_ptr = obj_ptr->GetTypePtr();
                
                ASSERT(type_ptr != nullptr);
                ASSERT_MSG(index < type_ptr->GetSize(), "member index out of bounds");
                
                thread->m_regs[dst] = obj_ptr->GetMember(index).value;
            } else {
                state->ThrowException(
                    thread,
                    Exception(utf::Utf8String("not a standard object"))
                );
            }
        }
    }

    inline void LoadMemHash(uint8_t dst, uint8_t src, uint32_t hash)
    {
        Value &sv = thread->m_regs[src];
        ASSERT(sv.m_type == Value::HEAP_POINTER);

        HeapValue *hv = sv.m_value.ptr;
        if (hv == nullptr) {
            state->ThrowException(thread, Exception::NullReferenceException());
        } else {
            if (Object *objptr = hv->GetPointer<Object>()) {
                if (Member *member = objptr->LookupMemberFromHash(hash)) {
                    thread->m_regs[dst] = member->value;
                } else {
                    state->ThrowException(
                        thread,
                        Exception::MemberNotFoundException()
                    );
                }
            } else {
                state->ThrowException(
                    thread,
                    Exception(utf::Utf8String("not a standard object"))
                );
            }
        }
    }

    inline void LoadArrayIdx(uint8_t dst, uint8_t src, uint8_t index_reg)
    {
        Value &sv = thread->m_regs[src];
        ASSERT_MSG(sv.m_type == Value::HEAP_POINTER, "source must be a pointer");

        Value &idx_sv = thread->m_regs[index_reg];

        if (HeapValue *hv = sv.m_value.ptr) {
            union {
                aint64 index;
                utf::Utf8String *str_ptr;
            };

            if (idx_sv.GetInteger(&index)) {
                if (Array *arrayptr = hv->GetPointer<Array>()) {
                    if (index >= arrayptr->GetSize()) {
                        state->ThrowException(thread, Exception::OutOfBoundsException());
                    } else if (index < 0) {
                        // wrap around (python style)
                        index = arrayptr->GetSize() + index;
                        if (index < 0 || index >= arrayptr->GetSize()) {
                            state->ThrowException(thread, Exception::OutOfBoundsException());
                        } else {
                            thread->m_regs[dst] = arrayptr->AtIndex(index);
                        }
                    } else {
                        thread->m_regs[dst] = arrayptr->AtIndex(index);
                    }
                } else {
                    state->ThrowException(thread,
                        Exception(utf::Utf8String("object is not an array")));
                }
            } else if (IS_VALUE_STRING(idx_sv, str_ptr)) {
                state->ThrowException(thread,
                    Exception(utf::Utf8String("Map is not yet supported")));
            } else {
                state->ThrowException(thread,
                    Exception(utf::Utf8String("array index must be of type Int or String")));
            }
        } else {
            state->ThrowException(thread, Exception::NullReferenceException());
        }
    }

    inline void LoadNull(uint8_t reg)
    {
        Value &sv = thread->m_regs[reg];
        sv.m_type = Value::HEAP_POINTER;
        sv.m_value.ptr = nullptr;
    }

    inline void LoadTrue(uint8_t reg)
    {
        Value &sv = thread->m_regs[reg];
        sv.m_type = Value::BOOLEAN;
        sv.m_value.b = true;
    }

    inline void LoadFalse(uint8_t reg)
    {
        Value &sv = thread->m_regs[reg];
        sv.m_type = Value::BOOLEAN;
        sv.m_value.b = false;
    }

    inline void MovOffset(uint16_t offset, uint8_t reg)
    {
        // copy value from register to stack value at (sp - offset)
        thread->m_stack[thread->m_stack.GetStackPointer() - offset] =
            thread->m_regs[reg];
    }

    inline void MovIndex(uint16_t index, uint8_t reg)
    {
        // copy value from register to stack value at index
        // NOTE: storing on main thread
        state->MAIN_THREAD->m_stack[index] = thread->m_regs[reg];
    }

    inline void MovMem(uint8_t dst, uint8_t index, uint8_t src)
    {
        Value &sv = thread->m_regs[dst];
        ASSERT_MSG(sv.m_type == Value::HEAP_POINTER, "destination must be a pointer");

        if (HeapValue *hv = sv.m_value.ptr) {
            if (Object *objptr = hv->GetPointer<Object>()) {
                const vm::TypeInfo *type_ptr = objptr->GetTypePtr();
                
                ASSERT(type_ptr != nullptr);
                ASSERT_MSG(index < type_ptr->GetSize(), "member index out of bounds");
                
                objptr->GetMember(index).value = thread->m_regs[src];
            } else {
                state->ThrowException(thread,
                    Exception(utf::Utf8String("not a standard object")));
            }
        } else {
            state->ThrowException(thread, Exception::NullReferenceException());
        }
    }

    inline void MovMemHash(uint8_t dst, uint32_t hash, uint8_t src)
    {
        Value &sv = thread->m_regs[dst];
        ASSERT_MSG(sv.m_type == Value::HEAP_POINTER, "destination must be a pointer");

        HeapValue *hv = sv.m_value.ptr;

        if (hv == nullptr) {
            state->ThrowException(
                thread,
                Exception::NullReferenceException()
            );
        } else {
            if (Object *obj_ptr = hv->GetPointer<Object>()) {
                if (Member *member = obj_ptr->LookupMemberFromHash(hash)) {
                    // set value in member
                    member->value = thread->m_regs[src];
                } else {
                    state->ThrowException(
                        thread,
                        Exception::MemberNotFoundException()
                    );
                }
            } else {
                state->ThrowException(
                    thread,
                    Exception(utf::Utf8String("not a standard object"))
                );
            }
        }
    }

    inline void MovArrayIdx(uint8_t dst, uint32_t index, uint8_t src)
    {
        Value &sv = thread->m_regs[dst];
        ASSERT_MSG(sv.m_type == Value::HEAP_POINTER, "destination must be a pointer");

        if (HeapValue *hv = sv.m_value.ptr) {
            if (Array *arrayptr = hv->GetPointer<Array>()) {
                if (index >= arrayptr->GetSize()) {
                    state->ThrowException(thread, Exception::OutOfBoundsException());
                } else {
                    arrayptr->AtIndex(index) = thread->m_regs[src];
                }
            } else {
                state->ThrowException(thread,
                    Exception(utf::Utf8String("object is not an array")));
            }
        } else {
            state->ThrowException(thread, Exception::NullReferenceException());
        }
    }

    inline void MovReg(uint8_t dst, uint8_t src)
    {
        thread->m_regs[dst] = thread->m_regs[src];
    }

    inline void HashMemHash(uint8_t dst, uint8_t src, uint32_t hash)
    {
        Value &sv = thread->m_regs[src];
        ASSERT_MSG(sv.m_type == Value::HEAP_POINTER, "destination must be a pointer");

        Value &res = thread->m_regs[dst];
        res.m_type = Value::BOOLEAN;

        if (sv.m_value.ptr != nullptr) {
            if (Object *obj_ptr = sv.m_value.ptr->GetPointer<Object>()) {
                if (obj_ptr->LookupMemberFromHash(hash) != nullptr) {
                    res.m_value.b = true;
                    // leave the case statement
                    return;
                }
            }
        }

        // not found, set it to false
        res.m_value.b = false;
    }

    inline void Push(uint8_t reg)
    {
        // push a copy of the register value to the top of the stack
        thread->m_stack.Push(thread->m_regs[reg]);
    }

    inline void Pop(ExecutionThread *thread)
    {
        thread->m_stack.Pop();
    }

    inline void PopN(uint8_t n)
    {
        thread->m_stack.Pop(n);
    }

    inline void PushArray(uint8_t dst, uint8_t src)
    {
        Value &sv = thread->m_regs[dst];
        ASSERT_MSG(sv.m_type == Value::HEAP_POINTER, "destination must be a pointer");

        if (HeapValue *hv = sv.m_value.ptr) {
            if (Array *arrayptr = hv->GetPointer<Array>()) {
                arrayptr->Push(thread->m_regs[src]);
            } else {
                state->ThrowException(thread,
                    Exception(utf::Utf8String("object is not an array")));
            }
        } else {
            state->ThrowException(thread,
                Exception::NullReferenceException());
        }
    }

    inline void Echo(uint8_t reg)
    {
        VM::Print(thread->m_regs[reg]);
    }

    inline void EchoNewline(ExecutionThread *thread)
    {
        utf::fputs(UTF8_CSTR("\n"), stdout);
    }

    inline void Jmp(uint8_t reg)
    {
        const Value &addr = thread->m_regs[reg];
        ASSERT_MSG(addr.m_type == Value::ADDRESS, "register must hold an address");

        bs->Seek(addr.m_value.addr);
    }

    inline void Je(uint8_t reg)
    {
        if (thread->m_regs.m_flags == EQUAL) {
            const Value &addr = thread->m_regs[reg];
            ASSERT_MSG(addr.m_type == Value::ADDRESS, "register must hold an address");

            bs->Seek(addr.m_value.addr);
        }
    }

    inline void Jne(uint8_t reg)
    {
        if (thread->m_regs.m_flags != EQUAL) {
            const Value &addr = thread->m_regs[reg];
            ASSERT_MSG(addr.m_type == Value::ADDRESS, "register must hold an address");

            bs->Seek(addr.m_value.addr);
        }
    }

    inline void Jg(uint8_t reg)
    {
        if (thread->m_regs.m_flags == GREATER) {
            const Value &addr = thread->m_regs[reg];
            ASSERT_MSG(addr.m_type == Value::ADDRESS, "register must hold an address");

            bs->Seek(addr.m_value.addr);
        }
    }

    inline void Jge(uint8_t reg)
    {
        if (thread->m_regs.m_flags == GREATER || thread->m_regs.m_flags == EQUAL) {
            const Value &addr = thread->m_regs[reg];
            ASSERT_MSG(addr.m_type == Value::ADDRESS, "register must hold an address");

            bs->Seek(addr.m_value.addr);
        }
    }

    inline void Call(uint8_t reg, uint8_t nargs)
    {
        VM::Invoke(
            this,
            thread->m_regs[reg],
            nargs
        );
    }

    inline void Ret(ExecutionThread *thread)
    {
        // get top of stack (should be the address before jumping)
        Value &top = thread->GetStack().Top();
        ASSERT(top.GetType() == Value::FUNCTION_CALL);
        
        // leave function and return to previous position
        bs->Seek(top.GetValue().call.addr);

        // increase stack size by the amount required by the call
        thread->GetStack().m_sp += top.GetValue().call.varargs_push - 1;
        // NOTE: the -1 is because we will be popping the FUNCTION_CALL 
        // object from the stack anyway...

        // decrease function depth
        thread->m_func_depth--;
    }

    inline void BeginTry(uint8_t reg)
    {
        // copy the value of the address for the catch-block
        const Value &catch_address = thread->m_regs[reg];
        ASSERT_MSG(catch_address.m_type == Value::ADDRESS, "register must hold an address");

        thread->m_exception_state.m_try_counter++;

        // increase stack size to store data about this try block
        Value info;
        info.m_type = Value::TRY_CATCH_INFO;
        info.m_value.try_catch_info.catch_address = catch_address.m_value.addr;

        // store the info
        thread->m_stack.Push(info);
    }

    inline void EndTry(ExecutionThread *thread)
    {
        // pop the try catch info from the stack
        ASSERT(thread->m_stack.Top().m_type == Value::TRY_CATCH_INFO);
        ASSERT(thread->m_exception_state.m_try_counter < 0);

        thread->m_stack.Pop();
        thread->m_exception_state.m_try_counter--;
    }

    inline void New(uint8_t dst, uint8_t src)
    {
        // read value from register
        Value &type_sv = thread->m_regs[src];
        ASSERT(type_sv.m_type == Value::HEAP_POINTER);

        TypeInfo *type_ptr = type_sv.m_value.ptr->GetPointer<TypeInfo>();
        ASSERT(type_ptr != nullptr);

        // allocate heap object
        HeapValue *hv = state->HeapAlloc(thread);
        ASSERT(hv != nullptr);
        
        // create the Object from the info type_ptr provides us with.
        hv->Assign(Object(type_ptr, type_sv));

        // assign register value to the allocated object
        Value &sv = thread->m_regs[dst];
        sv.m_type = Value::HEAP_POINTER;
        sv.m_value.ptr = hv;
    }

    inline void NewArray(uint8_t dst, uint32_t size)
    {
        // allocate heap object
        HeapValue *hv = state->HeapAlloc(thread);
        ASSERT(hv != nullptr);

        hv->Assign(Array(size));

        // assign register value to the allocated object
        Value &sv = thread->m_regs[dst];
        sv.m_type = Value::HEAP_POINTER;
        sv.m_value.ptr = hv;
    }

    inline void Cmp(uint8_t lhs_reg,
        uint8_t rhs_reg)
    {
        // dropout early for comparing something against itself
        if (lhs_reg == rhs_reg) {
            thread->m_regs.m_flags = EQUAL;
            return;
        }

        // load values from registers
        Value *lhs = &thread->m_regs[lhs_reg];
        Value *rhs = &thread->m_regs[rhs_reg];

        union {
            aint64 i;
            afloat64 f;
        } a, b;

        // compare integers
        if (lhs->GetInteger(&a.i) && rhs->GetInteger(&b.i)) {
            thread->m_regs.m_flags = (a.i == b.i) ? EQUAL : ((a.i > b.i) ? GREATER : NONE);
        } else if (lhs->GetNumber(&a.f) && rhs->GetNumber(&b.f)) {
            thread->m_regs.m_flags = (a.f == b.f) ? EQUAL : ((a.f > b.f) ? GREATER : NONE);
        } else if (lhs->m_type == Value::BOOLEAN && rhs->m_type == Value::BOOLEAN) {
            thread->m_regs.m_flags = (lhs->m_value.b == rhs->m_value.b) ? EQUAL 
                : ((lhs->m_value.b > rhs->m_value.b) ? GREATER : NONE);
        } else if (lhs->m_type == Value::HEAP_POINTER || rhs->m_type == Value::HEAP_POINTER) {
            int res = VM::CompareAsPointers(lhs, rhs);
            if (res != -1) {
                thread->m_regs.m_flags = res;
            } else {
                state->ThrowException(
                    thread,
                    Exception::InvalidComparisonException(
                        lhs->GetTypeString(),
                        rhs->GetTypeString()
                    )
                );
            }
        } else if (lhs->m_type == Value::FUNCTION && rhs->m_type == Value::FUNCTION) {
            thread->m_regs.m_flags = VM::CompareAsFunctions(lhs, rhs);
        } else if (lhs->m_type == Value::NATIVE_FUNCTION && rhs->m_type == Value::NATIVE_FUNCTION) {
            thread->m_regs.m_flags = VM::CompareAsNativeFunctions(lhs, rhs);
        } else {
            state->ThrowException(
                thread,
                Exception::InvalidComparisonException(
                    lhs->GetTypeString(),
                    rhs->GetTypeString()
                )
            );
        }
    }

    inline void CmpZ(uint8_t reg)
    {
        // load values from registers
        Value *lhs = &thread->m_regs[reg];

        union {
            aint64 i;
            afloat64 f;
        };

        if (lhs->GetInteger(&i)) {
            thread->m_regs.m_flags = !i ? EQUAL : NONE;
        } else if (lhs->GetFloatingPoint(&f)) {
            thread->m_regs.m_flags = !f ? EQUAL : NONE;
        } else if (lhs->m_type == Value::BOOLEAN) {
            thread->m_regs.m_flags = !lhs->m_value.b ? EQUAL : NONE;
        } else if (lhs->m_type == Value::HEAP_POINTER) {
            thread->m_regs.m_flags = !lhs->m_value.ptr ? EQUAL : NONE;
        } else if (lhs->m_type == Value::FUNCTION) {
            thread->m_regs.m_flags = NONE;
        } else {
            char buffer[256];
            std::sprintf(buffer, "cannot determine if type '%s' is non-zero", lhs->GetTypeString());
            state->ThrowException(thread, Exception(utf::Utf8String(buffer)));
        }
    }

    inline void Add(uint8_t lhs_reg,
        uint8_t rhs_reg,
        uint8_t dst_reg)
    {
        // load values from registers
        Value *lhs = &thread->m_regs[lhs_reg];
        Value *rhs = &thread->m_regs[rhs_reg];

        Value result;
        result.m_type = MATCH_TYPES(lhs->m_type, rhs->m_type);

        union {
            aint64 i;
            afloat64 f;
        } a, b;

        if (lhs->GetInteger(&a.i) && rhs->GetInteger(&b.i)) {
            aint64 result_value = a.i + b.i;
            if (result.m_type == Value::I32) {
                result.m_value.i32 = result_value;
            } else {
                result.m_value.i64 = result_value;
            }
        } else if (lhs->GetNumber(&a.f) && rhs->GetNumber(&b.f)) {
            afloat64 result_value = a.f + b.f;
            if (result.m_type == Value::F32) {
                result.m_value.f = result_value;
            } else {
                result.m_value.d = result_value;
            }
        } else {
            char buffer[256];
            std::sprintf(buffer, "cannot add types '%s' and '%s'",
                lhs->GetTypeString(), rhs->GetTypeString());
            state->ThrowException(thread, Exception(utf::Utf8String(buffer)));
        }

        // set the destination register to be the result
        thread->m_regs[dst_reg] = result;
    }

    inline void Sub(uint8_t lhs_reg,
        uint8_t rhs_reg,
        uint8_t dst_reg)
    {
        // load values from registers
        Value *lhs = &thread->m_regs[lhs_reg];
        Value *rhs = &thread->m_regs[rhs_reg];

        Value result;
        result.m_type = MATCH_TYPES(lhs->m_type, rhs->m_type);

        union {
            aint64 i;
            afloat64 f;
        } a, b;

        if (lhs->GetInteger(&a.i) && rhs->GetInteger(&b.i)) {
            aint64 result_value = a.i - b.i;
            if (result.m_type == Value::I32) {
                result.m_value.i32 = result_value;
            } else {
                result.m_value.i64 = result_value;
            }
        } else if (lhs->GetNumber(&a.f) && rhs->GetNumber(&b.f)) {
            afloat64 result_value = a.f - b.f;
            if (result.m_type == Value::F32) {
                result.m_value.f = result_value;
            } else {
                result.m_value.d = result_value;
            }
        } else {
            char buffer[256];
            std::sprintf(buffer, "cannot subtract types '%s' and '%s'",
                lhs->GetTypeString(), rhs->GetTypeString());
            state->ThrowException(thread, Exception(utf::Utf8String(buffer)));
        }

        // set the destination register to be the result
        thread->m_regs[dst_reg] = result;
    }

    inline void Mul(uint8_t lhs_reg,
        uint8_t rhs_reg,
        uint8_t dst_reg)
    {
        // load values from registers
        Value *lhs = &thread->m_regs[lhs_reg];
        Value *rhs = &thread->m_regs[rhs_reg];

        Value result;
        result.m_type = MATCH_TYPES(lhs->m_type, rhs->m_type);

        union {
            aint64 i;
            afloat64 f;
        } a, b;

        if (lhs->GetInteger(&a.i) && rhs->GetInteger(&b.i)) {
            aint64 result_value = a.i * b.i;
            if (result.m_type == Value::I32) {
                result.m_value.i32 = result_value;
            } else {
                result.m_value.i64 = result_value;
            }
        } else if (lhs->GetNumber(&a.f) && rhs->GetNumber(&b.f)) {
            afloat64 result_value = a.f * b.f;
            if (result.m_type == Value::F32) {
                result.m_value.f = result_value;
            } else {
                result.m_value.d = result_value;
            }
        } else {
            char buffer[256];
            std::sprintf(buffer, "cannot multiply types '%s' and '%s'",
                lhs->GetTypeString(), rhs->GetTypeString());
            state->ThrowException(thread, Exception(utf::Utf8String(buffer)));
        }

        // set the destination register to be the result
        thread->m_regs[dst_reg] = result;
    }

    inline void Div(uint8_t lhs_reg,
        uint8_t rhs_reg,
        uint8_t dst_reg)
    {
        // load values from registers
        Value *lhs = &thread->m_regs[lhs_reg];
        Value *rhs = &thread->m_regs[rhs_reg];

        Value result;
        result.m_type = MATCH_TYPES(lhs->m_type, rhs->m_type);

        union {
            aint64 i;
            afloat64 f;
        } a, b;

        if (lhs->GetInteger(&a.i) && rhs->GetInteger(&b.i)) {
            if (b.i == 0) {
                // division by zero
                state->ThrowException(thread, Exception::DivisionByZeroException());
            } else {
                aint64 result_value = a.i / b.i;
                if (result.m_type == Value::I32) {
                    result.m_value.i32 = result_value;
                } else {
                    result.m_value.i64 = result_value;
                }
            }
        } else if (lhs->GetNumber(&a.f) && rhs->GetNumber(&b.f)) {
            if (b.f == 0.0) {
                // division by zero
                state->ThrowException(thread, Exception::DivisionByZeroException());
            } else {
                afloat64 result_value = a.f / b.f;
                if (result.m_type == Value::F32) {
                    result.m_value.f = result_value;
                } else {
                    result.m_value.d = result_value;
                }
            }
        } else {
            char buffer[256];
            std::sprintf(buffer, "cannot divide types '%s' and '%s'",
                lhs->GetTypeString(), rhs->GetTypeString());
            state->ThrowException(thread, Exception(utf::Utf8String(buffer)));
        }

        // set the destination register to be the result
        thread->m_regs[dst_reg] = result;
    }

    inline void Neg(uint8_t reg)
    {
        // load value from register
        Value *value = &thread->m_regs[reg];

        union {
            aint64 i;
            afloat64 f;
        };

        if (value->GetInteger(&i)) {
            if (value->m_type == Value::I32) {
                value->m_value.i32 = -i;
            } else {
                value->m_value.i64 = -i;
            }
        } else if (value->GetFloatingPoint(&f)) {
            if (value->m_type == Value::F32) {
                value->m_value.f = -f;
            } else {
                value->m_value.d = -f;
            }
        } else {
            char buffer[256];
            std::sprintf(buffer, "cannot negate type '%s'", value->GetTypeString());
            state->ThrowException(thread, Exception(utf::Utf8String(buffer)));
        }
    }

    #if 0
    inline void StoreStaticString(uint32_t len, const char *str);
    void StoreStaticAddress(uint32_t addr);
    void StoreStaticFunction(uint32_t addr,
        uint8_t nargs, uint8_t is_variadic);
    void StoreStaticType(const char *type_name,
        uint16_t size, char **names);
    void LoadI32(uint8_t reg, aint32 i32);
    void LoadI64(uint8_t reg, aint64 i64);
    void LoadF32(uint8_t reg, afloat32 f32);
    void LoadF64(uint8_t reg, afloat64 f64);
    void LoadOffset(uint8_t reg, uint8_t offset);
    void LoadIndex(uint8_t reg, uint16_t index);
    void LoadStatic(uint8_t reg, uint16_t index);
    void LoadString(uint8_t reg, uint32_t len, const char *str);
    void LoadAddr(uint8_t reg, uint8_t addr);
    void LoadFunc(uint8_t reg, uint32_t addr,
        uint8_t nargs, uint8_t is_variadic);
    void LoadType(uint8_t reg, uint16_t type_name_len,
        const char *type_name, uint16_t size, char **names);
    void LoadMem(uint8_t dst, uint8_t src, uint8_t index);
    void LoadMemHash(uint8_t dst, uint8_t src, uint32_t hash);
    void LoadArrayIdx(uint8_t dst, uint8_t src, uint8_t index_reg);
    void LoadNull(uint8_t reg);
    void LoadTrue(uint8_t reg);
    void LoadFalse(uint8_t reg);
    void MovOffset(uint16_t offset, uint8_t reg);
    void MovIndex(uint16_t index, uint8_t reg);
    void MovMem(uint8_t dst, uint8_t index, uint8_t src);
    void MovMemHash(uint8_t dst, uint32_t hash, uint8_t src);
    void MovArrayIdx(uint8_t dst, uint32_t index, uint8_t src);
    void MovReg(uint8_t dst, uint8_t src);
    void HashMemHash(uint8_t dst, uint8_t src, uint32_t hash);
    void Push(uint8_t reg);
    void Pop(ExecutionThread *thread);
    void PopN(uint8_t n);
    void PushArray(uint8_t dst, uint8_t src);
    void Echo(uint8_t reg);
    void EchoNewline(ExecutionThread *thread);
    void Jmp(uint8_t reg);
    void Je(uint8_t reg);
    void Jne(uint8_t reg);
    void Jg(uint8_t reg);
    void Jge(uint8_t reg);
    void Call(uint8_t reg, uint8_t nargs);
    void Ret(ExecutionThread *thread);
    void BeginTry(uint8_t reg);
    void EndTry(ExecutionThread *thread);
    void New(uint8_t dst, uint8_t src);
    void NewArray(uint8_t dst, uint32_t size);
    void Cmp(uint8_t lhs_reg, uint8_t rhs_reg);
    void CmpZ(uint8_t reg);
    void Add(uint8_t lhs_reg, uint8_t rhs_reg, uint8_t dst_reg);
    void Sub(uint8_t lhs_reg, uint8_t rhs_reg, uint8_t dst_reg);
    void Mul(uint8_t lhs_reg, uint8_t rhs_reg, uint8_t dst_reg);
    void Div(uint8_t lhs_reg, uint8_t rhs_reg, uint8_t dst_reg);
    void Neg(uint8_t reg);

    #endif
};

} // namespace vm
} // namespace ace

#endif