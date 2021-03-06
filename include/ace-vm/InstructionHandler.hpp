#ifndef INSTRUCTION_HANDLER_HPP
#define INSTRUCTION_HANDLER_HPP

#include <ace-vm/BytecodeStream.hpp>
#include <ace-vm/VM.hpp>
#include <ace-vm/Value.hpp>
#include <ace-vm/HeapValue.hpp>
#include <ace-vm/Array.hpp>
#include <ace-vm/Object.hpp>
#include <ace-vm/ImmutableString.hpp>
#include <ace-vm/TypeInfo.hpp>

#include <common/instructions.hpp>
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
        hv->Assign(ImmutableString(str));

        Value sv;
        sv.m_type = Value::HEAP_POINTER;
        sv.m_value.ptr = hv;

        state->m_static_memory.Store(std::move(sv));
    }

    inline void StoreStaticAddress(bc_address_t addr)
    {
        Value sv;
        sv.m_type = Value::ADDRESS;
        sv.m_value.addr = addr;

        state->m_static_memory.Store(std::move(sv));
    }

    inline void StoreStaticFunction(bc_address_t addr,
        uint8_t nargs,
        uint8_t flags)
    {
        Value sv;
        sv.m_type = Value::FUNCTION;
        sv.m_value.func.m_addr = addr;
        sv.m_value.func.m_nargs = nargs;
        sv.m_value.func.m_flags = flags;

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

    inline void LoadI32(bc_reg_t reg, aint32 i32)
    {
        // get register value given
        Value &value = thread->m_regs[reg];
        value.m_type = Value::I32;
        value.m_value.i32 = i32;
    }

    inline void LoadI64(bc_reg_t reg, aint64 i64)
    {
        // get register value given
        Value &value = thread->m_regs[reg];
        value.m_type = Value::I64;
        value.m_value.i64 = i64;
    }

    inline void LoadF32(bc_reg_t reg, afloat32 f32)
    {
        // get register value given
        Value &value = thread->m_regs[reg];
        value.m_type = Value::F32;
        value.m_value.f = f32;
    }

    inline void LoadF64(bc_reg_t reg, afloat64 f64)
    {
        // get register value given
        Value &value = thread->m_regs[reg];
        value.m_type = Value::F64;
        value.m_value.d = f64;
    }

    inline void LoadOffset(bc_reg_t reg, uint16_t offset)
    {
        // read value from stack at (sp - offset)
        // into the the register
        thread->m_regs[reg] = thread->m_stack[thread->m_stack.GetStackPointer() - offset];
    }

    inline void LoadIndex(bc_reg_t reg, uint16_t index)
    {
        // read value from stack at the index into the the register
        // NOTE: read from main thread
        thread->m_regs[reg] = state->MAIN_THREAD->m_stack[index];
    }

    inline void LoadStatic(bc_reg_t reg, uint16_t index)
    {
        // read value from static memory
        // at the index into the the register
        thread->m_regs[reg] = state->m_static_memory[index];
    }

    inline void LoadString(bc_reg_t reg, uint32_t len, const char *str)
    {
        // allocate heap value
        HeapValue *hv = state->HeapAlloc(thread);
        if (hv != nullptr) {
            hv->Assign(ImmutableString(str));

            // assign register value to the allocated object
            Value &sv = thread->m_regs[reg];
            sv.m_type = Value::HEAP_POINTER;
            sv.m_value.ptr = hv;
        }
    }

    inline void LoadAddr(bc_reg_t reg, bc_address_t addr)
    {
        Value &sv = thread->m_regs[reg];
        sv.m_type = Value::ADDRESS;
        sv.m_value.addr = addr;
    }

    inline void LoadFunc(bc_reg_t reg,
        bc_address_t addr,
        uint8_t nargs,
        uint8_t flags)
    {
        Value &sv = thread->m_regs[reg];
        sv.m_type = Value::FUNCTION;
        sv.m_value.func.m_addr = addr;
        sv.m_value.func.m_nargs = nargs;
        sv.m_value.func.m_flags = flags;
    }

    inline void LoadType(bc_reg_t reg,
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

    inline void LoadMem(bc_reg_t dst, bc_reg_t src, uint8_t index)
    {
        Value &sv = thread->m_regs[src];
        
        if (sv.m_type == Value::HEAP_POINTER) {
            HeapValue *hv = sv.m_value.ptr;
            if (hv == nullptr) {
                state->ThrowException(
                    thread,
                    Exception::NullReferenceException()
                );
                return;
            } else if (Object *obj_ptr = hv->GetPointer<Object>()) {
                const vm::TypeInfo *type_ptr = obj_ptr->GetTypePtr();
                
                ASSERT(type_ptr != nullptr);
                ASSERT(index < type_ptr->GetSize());
                
                thread->m_regs[dst] = obj_ptr->GetMember(index).value;
                return;
            }
        }

        state->ThrowException(
            thread,
            Exception("Not an Object")
        );
    }

    inline void LoadMemHash(bc_reg_t dst_reg, bc_reg_t src_reg, uint32_t hash)
    {
        Value &sv = thread->m_regs[src_reg];

        if (sv.m_type == Value::HEAP_POINTER) {
            HeapValue *hv = sv.m_value.ptr;

            if (hv == nullptr) {
                state->ThrowException(
                    thread,
                    Exception::NullReferenceException()
                );
                return;
            } else if (Object *object = hv->GetPointer<Object>()) {
                if (Member *member = object->LookupMemberFromHash(hash)) {
                    thread->m_regs[dst_reg] = member->value;
                } else {
                    state->ThrowException(
                        thread,
                        Exception::MemberNotFoundException()
                    );
                }
                return;
            }
        }

        state->ThrowException(
            thread,
            Exception("Not an Object")
        );
    }

    inline void LoadArrayIdx(bc_reg_t dst_reg, bc_reg_t src_reg, bc_reg_t index_reg)
    {
        Value &sv = thread->m_regs[src_reg];

        if (sv.m_type != Value::HEAP_POINTER) {
            state->ThrowException(
                thread,
                Exception("Not an Array")
            );
            return;
        }

        HeapValue *ptr = sv.m_value.ptr;
        if (ptr == nullptr) {
            state->ThrowException(
                thread,
                Exception::NullReferenceException()
            );
            return;
        }

        union {
            aint64 index;
            ImmutableString *str;
        } key;

        if (!thread->m_regs[index_reg].GetInteger(&key.index)) {
            state->ThrowException(
                thread,
                Exception("Array index must be of type Int or String")
            );
            return;
        }

        Array *array = ptr->GetPointer<Array>();
        if (array == nullptr) {
            state->ThrowException(
                thread,
                Exception("Not an Array")
            );
            return;
        }
        
        if ((size_t)key.index >= array->GetSize()) {
            state->ThrowException(
                thread,
                Exception::OutOfBoundsException()
            );
            return;
        }
        
        if (key.index < 0) {
            // wrap around (python style)
            key.index = (aint64)(array->GetSize() + key.index);
            if (key.index < 0 || (size_t)key.index >= array->GetSize()) {
                state->ThrowException(
                    thread,
                    Exception::OutOfBoundsException()
                );
                return;
            }
        }

        thread->m_regs[dst_reg] = array->AtIndex(key.index);
    }

    inline void LoadRef(bc_reg_t dst_reg, bc_reg_t src_reg)
    {
        Value &src = thread->m_regs[dst_reg];
        src.m_type = Value::VALUE_REF;
        src.m_value.value_ref = &thread->m_regs[src_reg];
    }

    inline void LoadDeref(bc_reg_t dst_reg, bc_reg_t src_reg)
    {
        Value &src = thread->m_regs[src_reg];
        ASSERT_MSG(src.m_type == Value::VALUE_REF, "Value type must be VALUE_REF in order to deref");
        ASSERT(src.m_value.value_ref != nullptr);

        thread->m_regs[dst_reg] = *src.m_value.value_ref;
    }

    inline void LoadNull(bc_reg_t reg)
    {
        Value &sv = thread->m_regs[reg];
        sv.m_type = Value::HEAP_POINTER;
        sv.m_value.ptr = nullptr;
    }

    inline void LoadTrue(bc_reg_t reg)
    {
        Value &sv = thread->m_regs[reg];
        sv.m_type = Value::BOOLEAN;
        sv.m_value.b = true;
    }

    inline void LoadFalse(bc_reg_t reg)
    {
        Value &sv = thread->m_regs[reg];
        sv.m_type = Value::BOOLEAN;
        sv.m_value.b = false;
    }

    inline void MovOffset(uint16_t offset, bc_reg_t reg)
    {
        // copy value from register to stack value at (sp - offset)
        thread->m_stack[thread->m_stack.GetStackPointer() - offset] =
            thread->m_regs[reg];
    }

    inline void MovIndex(uint16_t index, bc_reg_t reg)
    {
        // copy value from register to stack value at index
        // NOTE: storing on main thread
        state->MAIN_THREAD->m_stack[index] = thread->m_regs[reg];
    }

    inline void MovMem(bc_reg_t dst_reg, uint8_t index, bc_reg_t src_reg)
    {
        Value &sv = thread->m_regs[dst_reg];
        if (sv.m_type != Value::HEAP_POINTER) {
            state->ThrowException(
                thread,
                Exception("Not an Object")
            );
            return;
        }

        HeapValue *hv = sv.m_value.ptr;
        if (hv == nullptr) {
            state->ThrowException(
                thread,
                Exception::NullReferenceException()
            );
            return;
        }
        
        Object *object = hv->GetPointer<Object>();
        if (object == nullptr) {
            state->ThrowException(
                thread,
                Exception("Not an Object")
            );
            return;
        }

        const vm::TypeInfo *type_ptr = object->GetTypePtr();
        ASSERT(type_ptr != nullptr);

        if (index >= type_ptr->GetSize()) {
            state->ThrowException(
                thread,
                Exception::OutOfBoundsException()
            );
            return;
        }
        
        object->GetMember(index).value = thread->m_regs[src_reg];
    }

    inline void MovMemHash(bc_reg_t dst_reg, uint32_t hash, bc_reg_t src_reg)
    {
        Value &sv = thread->m_regs[dst_reg];

        if (sv.m_type != Value::HEAP_POINTER) {
            state->ThrowException(
                thread,
                Exception("Not an Object")
            );
            return;
        }

        HeapValue *hv = sv.m_value.ptr;
        if (hv == nullptr) {
            state->ThrowException(
                thread,
                Exception::NullReferenceException()
            );
            return;
        }

        Object *object = hv->GetPointer<Object>();
        if (object == nullptr) {
            state->ThrowException(
                thread,
                Exception("Not an Object")
            );
            return;
        }

        Member *member = object->LookupMemberFromHash(hash);
        if (member == nullptr) {
            state->ThrowException(
                thread,
                Exception::MemberNotFoundException()
            );
            return;
        }
        
        // set value in member
        member->value = thread->m_regs[src_reg];
    }

    inline void MovArrayIdx(bc_reg_t dst_reg, uint32_t index, bc_reg_t src_reg)
    {
        Value &sv = thread->m_regs[dst_reg];

        if (sv.m_type != Value::HEAP_POINTER) {
            state->ThrowException(
                thread,
                Exception("Not an Array")
            );
            return;
        }

        HeapValue *hv = sv.m_value.ptr;
        if (hv == nullptr) {
            state->ThrowException(
                thread,
                Exception::NullReferenceException()
            );
            return;
        }

        Array *array = hv->GetPointer<Array>();
        if (array == nullptr) {
            state->ThrowException(
                thread,
                Exception("Not an Array")
            );
            return;
        }

        if (index >= array->GetSize()) {
            state->ThrowException(
                thread,
                Exception::OutOfBoundsException()
            );
            return;
        }

        /*if (index < 0) {
            // wrap around (python style)
            index = array->GetSize() + index;
            if (index < 0 || index >= array->GetSize()) {
                state->ThrowException(
                    thread,
                    Exception::OutOfBoundsException()
                );
                return;
            }
        }*/
        
        array->AtIndex(index) = thread->m_regs[src_reg];
    }

    inline void MovReg(bc_reg_t dst_reg, bc_reg_t src_reg)
    {
        thread->m_regs[dst_reg] = thread->m_regs[src_reg];
    }

    inline void HasMemHash(bc_reg_t dst_reg, bc_reg_t src_reg, uint32_t hash)
    {
        Value &src = thread->m_regs[src_reg];

        Value &dst = thread->m_regs[dst_reg];
        dst.m_type = Value::BOOLEAN;

        if (src.m_type == Value::HEAP_POINTER && src.m_value.ptr != nullptr) {
            if (Object *object = src.m_value.ptr->GetPointer<Object>()) {
                dst.m_value.b = (object->LookupMemberFromHash(hash) != nullptr);
                return;
            }
        }

        // not found, set it to false
        dst.m_value.b = false;
    }

    inline void Push(bc_reg_t reg)
    {
        // push a copy of the register value to the top of the stack
        thread->m_stack.Push(thread->m_regs[reg]);
    }

    inline void Pop()
    {
        thread->m_stack.Pop();
    }

    inline void PopN(uint8_t n)
    {
        thread->m_stack.Pop(n);
    }

    inline void PushArray(bc_reg_t dst_reg, bc_reg_t src_reg)
    {
        Value &dst = thread->m_regs[dst_reg];
        if (dst.m_type != Value::HEAP_POINTER) {
            state->ThrowException(
                thread,
                Exception("Not an Array")
            );
            return;
        }

        HeapValue *hv = dst.m_value.ptr;
        if (hv == nullptr) {
            state->ThrowException(
                thread,
                Exception::NullReferenceException()
            );
            return;
        }

        Array *array = hv->GetPointer<Array>();
        if (array == nullptr) {
            state->ThrowException(
                thread,
                Exception("Not an Array")
            );
            return;
        }

        array->Push(thread->m_regs[src_reg]);
    }

    inline void Echo(bc_reg_t reg)
    {
        VM::Print(thread->m_regs[reg]);
    }

    inline void EchoNewline()
    {
        utf::fputs(UTF8_CSTR("\n"), stdout);
    }

    inline void Jmp(bc_address_t addr)
    {
        bs->Seek(addr);
    }

    inline void Je(bc_address_t addr)
    {
        if (thread->m_regs.m_flags == EQUAL) {
            bs->Seek(addr);
        }
    }

    inline void Jne(bc_address_t addr)
    {
        if (thread->m_regs.m_flags != EQUAL) {
            bs->Seek(addr);
        }
    }

    inline void Jg(bc_address_t addr)
    {
        if (thread->m_regs.m_flags == GREATER) {
            bs->Seek(addr);
        }
    }

    inline void Jge(bc_address_t addr)
    {
        if (thread->m_regs.m_flags == GREATER || thread->m_regs.m_flags == EQUAL) {
            bs->Seek(addr);
        }
    }

    inline void Call(bc_reg_t reg, uint8_t nargs)
    {
        VM::Invoke(
            this,
            thread->m_regs[reg],
            nargs
        );
    }

    inline void Ret()
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

    inline void BeginTry(bc_address_t addr)
    {
        thread->m_exception_state.m_try_counter++;

        // increase stack size to store data about this try block
        Value info;
        info.m_type = Value::TRY_CATCH_INFO;
        info.m_value.try_catch_info.catch_address = addr;

        // store the info
        thread->m_stack.Push(info);
    }

    inline void EndTry()
    {
        // pop the try catch info from the stack
        ASSERT(thread->m_stack.Top().m_type == Value::TRY_CATCH_INFO);
        ASSERT(thread->m_exception_state.m_try_counter > 0);

        // pop try catch info
        thread->m_stack.Pop();
        thread->m_exception_state.m_try_counter--;
    }

    inline void New(bc_reg_t dst, bc_reg_t src)
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

    inline void NewArray(bc_reg_t dst, uint32_t size)
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

    inline void Cmp(bc_reg_t lhs_reg, bc_reg_t rhs_reg)
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
            thread->m_regs.m_flags = (a.i == b.i)
                ? EQUAL : ((a.i > b.i)
                ? GREATER : NONE);
        } else if (lhs->GetNumber(&a.f) && rhs->GetNumber(&b.f)) {
            thread->m_regs.m_flags = (a.f == b.f)
                ? EQUAL : ((a.f > b.f)
                ? GREATER : NONE);
        } else if (lhs->m_type == Value::BOOLEAN && rhs->m_type == Value::BOOLEAN) {
            thread->m_regs.m_flags = (lhs->m_value.b == rhs->m_value.b)
                ? EQUAL : ((lhs->m_value.b > rhs->m_value.b)
                ? GREATER : NONE);
        } else if (lhs->m_type == Value::HEAP_POINTER && rhs->m_type == Value::HEAP_POINTER) {
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

    inline void CmpZ(bc_reg_t reg)
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
            // functions are never null
            thread->m_regs.m_flags = NONE;
        } else {
            char buffer[256];
            std::sprintf(
                buffer,
                "Cannot determine if type '%s' is non-zero",
                lhs->GetTypeString()
            );
            state->ThrowException(
                thread,
                Exception(buffer)
            );
        }
    }

    inline void Add(bc_reg_t lhs_reg,
        bc_reg_t rhs_reg,
        bc_reg_t dst_reg)
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
            state->ThrowException(
                thread,
                Exception::InvalidOperationException(
                    "ADD",
                    lhs->GetTypeString(),
                    rhs->GetTypeString()
                )
            );
            return;
        }

        // set the destination register to be the result
        thread->m_regs[dst_reg] = result;
    }

    inline void Sub(bc_reg_t lhs_reg,
        bc_reg_t rhs_reg,
        bc_reg_t dst_reg)
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
            state->ThrowException(
                thread,
                Exception::InvalidOperationException(
                    "SUB",
                    lhs->GetTypeString(),
                    rhs->GetTypeString()
                )
            );
            return;
        }

        // set the destination register to be the result
        thread->m_regs[dst_reg] = result;
    }

    inline void Mul(bc_reg_t lhs_reg,
        bc_reg_t rhs_reg,
        bc_reg_t dst_reg)
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
            state->ThrowException(
                thread,
                Exception::InvalidOperationException(
                    "MUL",
                    lhs->GetTypeString(),
                    rhs->GetTypeString()
                )
            );
            return;
        }

        // set the destination register to be the result
        thread->m_regs[dst_reg] = result;
    }

    inline void Div(bc_reg_t lhs_reg, bc_reg_t rhs_reg, bc_reg_t dst_reg)
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
            state->ThrowException(
                thread,
                Exception::InvalidOperationException(
                    "DIV",
                    lhs->GetTypeString(),
                    rhs->GetTypeString()
                )
            );
            return;
        }

        // set the destination register to be the result
        thread->m_regs[dst_reg] = result;
    }

    inline void Mod(bc_reg_t lhs_reg, bc_reg_t rhs_reg, bc_reg_t dst_reg)
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
                state->ThrowException(
                    thread,
                    Exception::DivisionByZeroException()
                );
            } else {
                aint64 result_value = a.i % b.i;
                if (result.m_type == Value::I32) {
                    result.m_value.i32 = result_value;
                } else {
                    result.m_value.i64 = result_value;
                }
            }
        } else if (lhs->GetNumber(&a.f) && rhs->GetNumber(&b.f)) {
            if (b.f == 0.0) {
                // division by zero
                state->ThrowException(
                    thread,
                    Exception::DivisionByZeroException()
                );
            } else {
                afloat64 result_value = std::fmod(a.f, b.f);
                if (result.m_type == Value::F32) {
                    result.m_value.f = result_value;
                } else {
                    result.m_value.d = result_value;
                }
            }
        } else {
            state->ThrowException(
                thread,
                Exception::InvalidOperationException(
                    "MOD",
                    lhs->GetTypeString(),
                    rhs->GetTypeString()
                )
            );
            return;
        }

        // set the destination register to be the result
        thread->m_regs[dst_reg] = result;
    }

    inline void And(bc_reg_t lhs_reg,
        bc_reg_t rhs_reg,
        bc_reg_t dst_reg)
    {
        // load values from registers
        Value *lhs = &thread->m_regs[lhs_reg];
        Value *rhs = &thread->m_regs[rhs_reg];

        Value result;
        result.m_type = MATCH_TYPES(lhs->m_type, rhs->m_type);

        aint64 a, b;

        if (lhs->GetInteger(&a) && rhs->GetInteger(&b)) {
            aint64 result_value = a & b;
            if (result.m_type == Value::I32) {
                result.m_value.i32 = result_value;
            } else {
                result.m_value.i64 = result_value;
            }
        } else {
            state->ThrowException(
                thread,
                Exception::InvalidOperationException(
                    "AND",
                    lhs->GetTypeString(),
                    rhs->GetTypeString()
                )
            );
            return;
        }

        // set the destination register to be the result
        thread->m_regs[dst_reg] = result;
    }

    inline void Or(bc_reg_t lhs_reg,
        bc_reg_t rhs_reg,
        bc_reg_t dst_reg)
    {
        // load values from registers
        Value *lhs = &thread->m_regs[lhs_reg];
        Value *rhs = &thread->m_regs[rhs_reg];

        Value result;
        result.m_type = MATCH_TYPES(lhs->m_type, rhs->m_type);

        aint64 a, b;

        if (lhs->GetInteger(&a) && rhs->GetInteger(&b)) {
            aint64 result_value = a | b;
            if (result.m_type == Value::I32) {
                result.m_value.i32 = result_value;
            } else {
                result.m_value.i64 = result_value;
            }
        } else {
            state->ThrowException(
                thread,
                Exception::InvalidOperationException(
                    "OR",
                    lhs->GetTypeString(),
                    rhs->GetTypeString()
                )
            );
            return;
        }

        // set the destination register to be the result
        thread->m_regs[dst_reg] = result;
    }

    inline void Xor(bc_reg_t lhs_reg,
        bc_reg_t rhs_reg,
        bc_reg_t dst_reg)
    {
        // load values from registers
        Value *lhs = &thread->m_regs[lhs_reg];
        Value *rhs = &thread->m_regs[rhs_reg];

        Value result;
        result.m_type = MATCH_TYPES(lhs->m_type, rhs->m_type);

        aint64 a, b;

        if (lhs->GetInteger(&a) && rhs->GetInteger(&b)) {
            aint64 result_value = a ^ b;
            if (result.m_type == Value::I32) {
                result.m_value.i32 = result_value;
            } else {
                result.m_value.i64 = result_value;
            }
        } else {
            state->ThrowException(
                thread,
                Exception::InvalidOperationException(
                    "XOR",
                    lhs->GetTypeString(),
                    rhs->GetTypeString()
                )
            );
            return;
        }

        // set the destination register to be the result
        thread->m_regs[dst_reg] = result;
    }

    inline void Shl(bc_reg_t lhs_reg,
        bc_reg_t rhs_reg,
        bc_reg_t dst_reg)
    {
        // load values from registers
        Value *lhs = &thread->m_regs[lhs_reg];
        Value *rhs = &thread->m_regs[rhs_reg];

        Value result;
        result.m_type = MATCH_TYPES(lhs->m_type, rhs->m_type);

        aint64 a, b;

        if (lhs->GetInteger(&a) && rhs->GetInteger(&b)) {
            aint64 result_value = a << b;
            if (result.m_type == Value::I32) {
                result.m_value.i32 = result_value;
            } else {
                result.m_value.i64 = result_value;
            }
        } else {
            state->ThrowException(
                thread,
                Exception::InvalidOperationException(
                    "SHL",
                    lhs->GetTypeString(),
                    rhs->GetTypeString()
                )
            );
            return;
        }

        // set the destination register to be the result
        thread->m_regs[dst_reg] = result;
    }

    inline void Shr(bc_reg_t lhs_reg,
        bc_reg_t rhs_reg,
        bc_reg_t dst_reg)
    {
        // load values from registers
        Value *lhs = &thread->m_regs[lhs_reg];
        Value *rhs = &thread->m_regs[rhs_reg];

        Value result;
        result.m_type = MATCH_TYPES(lhs->m_type, rhs->m_type);

        aint64 a, b;

        if (lhs->GetInteger(&a) && rhs->GetInteger(&b)) {
            aint64 result_value = a >> b;
            if (result.m_type == Value::I32) {
                result.m_value.i32 = result_value;
            } else {
                result.m_value.i64 = result_value;
            }
        } else {
            state->ThrowException(
                thread,
                Exception::InvalidOperationException(
                    "SHR",
                    lhs->GetTypeString(),
                    rhs->GetTypeString()
                )
            );
            return;
        }

        // set the destination register to be the result
        thread->m_regs[dst_reg] = result;
    }

    inline void Neg(bc_reg_t reg)
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
            state->ThrowException(
                thread,
                Exception::InvalidOperationException(
                    "NEG",
                    value->GetTypeString()
                )
            );
            return;
        }
    }

    #if 0
    inline void StoreStaticString(uint32_t len, const char *str);
    void StoreStaticAddress(bc_address_t addr);
    void StoreStaticFunction(bc_address_t addr,
        uint8_t nargs, uint8_t is_variadic);
    void StoreStaticType(const char *type_name,
        uint16_t size, char **names);
    void LoadI32(bc_reg_t reg, aint32 i32);
    void LoadI64(bc_reg_t reg, aint64 i64);
    void LoadF32(bc_reg_t reg, afloat32 f32);
    void LoadF64(bc_reg_t reg, afloat64 f64);
    void LoadOffset(bc_reg_t reg, uint8_t offset);
    void LoadIndex(bc_reg_t reg, uint16_t index);
    void LoadStatic(bc_reg_t reg, uint16_t index);
    void LoadString(bc_reg_t reg, uint32_t len, const char *str);
    void LoadAddr(bc_reg_t reg, uint8_t addr);
    void LoadFunc(bc_reg_t reg, bc_address_t addr,
        uint8_t nargs, uint8_t is_variadic);
    void LoadType(bc_reg_t reg, uint16_t type_name_len,
        const char *type_name, uint16_t size, char **names);
    void LoadMem(uint8_t dst, uint8_t src, uint8_t index);
    void LoadMemHash(uint8_t dst, uint8_t src, uint32_t hash);
    void LoadArrayIdx(uint8_t dst, uint8_t src, uint8_t index_reg);
    void LoadNull(bc_reg_t reg);
    void LoadTrue(bc_reg_t reg);
    void LoadFalse(bc_reg_t reg);
    void MovOffset(uint16_t offset, bc_reg_t reg);
    void MovIndex(uint16_t index, bc_reg_t reg);
    void MovMem(uint8_t dst, uint8_t index, uint8_t src);
    void MovMemHash(uint8_t dst, uint32_t hash, uint8_t src);
    void MovArrayIdx(uint8_t dst, uint32_t index, uint8_t src);
    void MovReg(uint8_t dst, uint8_t src);
    void HashMemHash(uint8_t dst, uint8_t src, uint32_t hash);
    void Push(bc_reg_t reg);
    void Pop(ExecutionThread *thread);
    void PopN(uint8_t n);
    void PushArray(uint8_t dst, uint8_t src);
    void Echo(bc_reg_t reg);
    void EchoNewline(ExecutionThread *thread);
    void Jmp(bc_reg_t reg);
    void Je(bc_reg_t reg);
    void Jne(bc_reg_t reg);
    void Jg(bc_reg_t reg);
    void Jge(bc_reg_t reg);
    void Call(bc_reg_t reg, uint8_t nargs);
    void Ret(ExecutionThread *thread);
    void BeginTry(bc_reg_t reg);
    void EndTry(ExecutionThread *thread);
    void New(uint8_t dst, uint8_t src);
    void NewArray(uint8_t dst, uint32_t size);
    void Cmp(uint8_t lhs_reg, uint8_t rhs_reg);
    void CmpZ(bc_reg_t reg);
    void Add(uint8_t lhs_reg, uint8_t rhs_reg, uint8_t dst_reg);
    void Sub(uint8_t lhs_reg, uint8_t rhs_reg, uint8_t dst_reg);
    void Mul(uint8_t lhs_reg, uint8_t rhs_reg, uint8_t dst_reg);
    void Div(uint8_t lhs_reg, uint8_t rhs_reg, uint8_t dst_reg);
    void Neg(bc_reg_t reg);

    #endif
};

} // namespace vm
} // namespace ace

#endif