#include <ace-vm/vm.hpp>
#include <ace-vm/stack_value.hpp>
#include <ace-vm/heap_value.hpp>
#include <ace-vm/array.hpp>
#include <ace-vm/object.hpp>
#include <ace-vm/type_info.hpp>

#include <common/instructions.hpp>
#include <common/utf8.hpp>

#include <algorithm>
#include <cstdio>
#include <common/my_assert.hpp>
#include <cinttypes>
#include <mutex>

static std::mutex mtx;

VM::VM()
{
    m_state.m_vm = non_owning_ptr<VM>(this);
    // create main thread
    m_state.CreateThread();
}

VM::~VM()
{
}

void VM::PushNativeFunctionPtr(NativeFunctionPtr_t ptr)
{
    ASSERT(m_state.GetNumThreads() > 0);

    StackValue sv;
    sv.m_type = StackValue::NATIVE_FUNCTION;
    sv.m_value.native_func = ptr;
    MAIN_THREAD->m_stack.Push(sv);
}

void VM::Print(const StackValue &value)
{
    switch (value.m_type) {
    case StackValue::INT32:
        utf::printf(UTF8_CSTR("%d"), value.m_value.i32);
        break;
    case StackValue::INT64:
        utf::printf(UTF8_CSTR("%" PRId64), value.m_value.i64);
        break;
    case StackValue::FLOAT:
        utf::printf(UTF8_CSTR("%g"), value.m_value.f);
        break;
    case StackValue::DOUBLE:
        utf::printf(UTF8_CSTR("%g"), value.m_value.d);
        break;
    case StackValue::BOOLEAN:
        utf::fputs(value.m_value.b ? UTF8_CSTR("true") : UTF8_CSTR("false"), stdout);
        break;
    case StackValue::HEAP_POINTER:
    {
        utf::Utf8String *str = nullptr;
        Object *objptr = nullptr;
        Array *arrayptr = nullptr;
        
        if (value.m_value.ptr == nullptr) {
            // special case for null pointers
            utf::fputs(UTF8_CSTR("null"), stdout);
        } else if ((str = value.m_value.ptr->GetPointer<utf::Utf8String>()) != nullptr) {
            // print string value
            utf::cout << *str;
        } else if ((arrayptr = value.m_value.ptr->GetPointer<Array>()) != nullptr) {
            // print array list
            const char sep_str[] = ", ";
            const size_t sep_str_len = std::strlen(sep_str);

            int buffer_index = 1;
            const int buffer_size = 256;
            utf::Utf8String res("[", buffer_size);

            // convert all array elements to string
            const int size = arrayptr->GetSize();
            for (int i = 0; i < size; i++) {
                utf::Utf8String item_str = arrayptr->AtIndex(i).ToString();
                size_t len = item_str.GetLength();

                bool last = i != size - 1;
                if (last) {
                    len += sep_str_len;
                }

                if (buffer_index + len < buffer_size - 5) {
                    buffer_index += len;
                    res += item_str;
                    if (last) {
                        res += sep_str;
                    }
                } else {
                    res += "... ";
                    break;
                }
            }

            res += "]";
            utf::cout << res;
        } else {
            utf::fputs(UTF8_CSTR("Object"), stdout);
        }

        break;
    }
    case StackValue::FUNCTION: utf::fputs(UTF8_CSTR("Function"), stdout); break;
    case StackValue::NATIVE_FUNCTION: utf::fputs(UTF8_CSTR("NativeFunction"), stdout); break;
    default: utf::fputs(UTF8_CSTR("??"), stdout); break;
    }
}

void VM::Invoke(ExecutionThread *thread, BytecodeStream *bs, const StackValue &value, uint8_t num_args)
{
    if (value.m_type != StackValue::FUNCTION) {
        if (value.m_type == StackValue::NATIVE_FUNCTION) {
            StackValue **args = new StackValue*[num_args > 0 ? num_args : 1];

            int i = (int)thread->m_stack.GetStackPointer() - 1;
            for (int j = num_args - 1; j >= 0 && i >= 0; i--, j--) {
                args[j] = &thread->m_stack[i];
            }

            value.m_value.native_func(&m_state, thread, args, num_args);

            delete[] args;
        } else {
            char buffer[256];
            std::sprintf(buffer, "cannot invoke type '%s' as a function",
                value.GetTypeString());
            m_state.ThrowException(thread, Exception(buffer));
        }
    } else if (value.m_value.func.m_nargs != num_args) {
        m_state.ThrowException(thread,
            Exception::InvalidArgsException(value.m_value.func.m_nargs, num_args));
    } else {
        // store current address
        StackValue previous_addr;
        previous_addr.m_type = StackValue::ADDRESS;
        previous_addr.m_value.addr = (uint32_t)bs->Position();
        
        thread->GetStack().Push(previous_addr);
        
        // seek to the new address
        bs->Seek(value.m_value.func.m_addr);

        // increase function depth
        thread->m_func_depth++;
    }
}

void VM::HandleInstruction(ExecutionThread *thread, BytecodeStream *bs, uint8_t code)
{
    std::lock_guard<std::mutex> lock(mtx);
    
    if (!m_state.good) {
        return;
    }

    switch (code) {
    case STORE_STATIC_STRING: {
        // get string length
        uint32_t len;
        bs->Read(&len);

        // read string based on length
        char *str = new char[len + 1];
        bs->Read(str, len);
        str[len] = '\0';

        // the value will be freed on
        // the destructor call of m_state.m_static_memory
        HeapValue *hv = new HeapValue();
        hv->Assign(utf::Utf8String(str));

        StackValue sv;
        sv.m_type = StackValue::HEAP_POINTER;
        sv.m_value.ptr = hv;

        m_state.m_static_memory.Store(std::move(sv));

        delete[] str;

        break;
    }
    case STORE_STATIC_ADDRESS: {
        uint32_t value;
        bs->Read(&value);

        StackValue sv;
        sv.m_type = StackValue::ADDRESS;
        sv.m_value.addr = value;

        m_state.m_static_memory.Store(std::move(sv));

        break;
    }
    case STORE_STATIC_FUNCTION: {
        uint32_t addr;
        bs->Read(&addr);

        uint8_t nargs;
        bs->Read(&nargs);

        StackValue sv;
        sv.m_type = StackValue::FUNCTION;
        sv.m_value.func.m_addr = addr;
        sv.m_value.func.m_nargs = nargs;

        m_state.m_static_memory.Store(std::move(sv));

        break;
    }
    case STORE_STATIC_TYPE: {
        uint16_t size;
        bs->Read(&size);

        ASSERT(size > 0);

        uint32_t *hashes = new uint32_t[size];

        // load (size) hashes.
        for (int i = 0; i < size; i++) {
            bs->Read(&hashes[i]);
        }

        // the value will be freed on
        // the destructor call of m_state.m_static_memory
        HeapValue *hv = new HeapValue();
        hv->Assign(TypeInfo(size, hashes));

        StackValue sv;
        sv.m_type = StackValue::HEAP_POINTER;
        sv.m_value.ptr = hv;
        m_state.m_static_memory.Store(std::move(sv));

        delete[] hashes;

        break;
    }
    case LOAD_I32: {
        uint8_t reg;
        bs->Read(&reg);

        // get register value given
        StackValue &value = thread->m_regs[reg];
        value.m_type = StackValue::INT32;

        // read 32-bit integer into register value
        bs->Read(&value.m_value.i32);

        break;
    }
    case LOAD_I64: {
        uint8_t reg;
        bs->Read(&reg);

        // get register value given
        StackValue &value = thread->m_regs[reg];
        value.m_type = StackValue::INT64;

        // read 64-bit integer into register value
        bs->Read(&value.m_value.i64);

        break;
    }
    case LOAD_F32: {
        uint8_t reg;
        bs->Read(&reg);

        // get register value given
        StackValue &value = thread->m_regs[reg];
        value.m_type = StackValue::FLOAT;

        // read float into register value
        bs->Read(&value.m_value.f);

        break;
    }
    case LOAD_F64: {
        uint8_t reg;
        bs->Read(&reg);

        // get register value given
        StackValue &value = thread->m_regs[reg];
        value.m_type = StackValue::DOUBLE;

        // read double into register value
        bs->Read(&value.m_value.d);

        break;
    }
    case LOAD_OFFSET: {
        uint8_t reg;
        bs->Read(&reg);

        uint16_t offset;
        bs->Read(&offset);

        // read value from stack at (sp - offset)
        // into the the register
        thread->m_regs[reg] = thread->m_stack[thread->m_stack.GetStackPointer() - offset];

        break;
    }
    case LOAD_INDEX: {
        uint8_t reg;
        bs->Read(&reg);

        uint16_t idx;
        bs->Read(&idx);

        // read value from stack at the index into the the register
        // NOTE: read from main thread
        thread->m_regs[reg] = MAIN_THREAD->m_stack[idx];

        break;
    }
    case LOAD_STATIC: {
        uint8_t reg;
        bs->Read(&reg);

        uint16_t index;
        bs->Read(&index);

        // read value from static memory
        // at the index into the the register
        thread->m_regs[reg] = m_state.m_static_memory[index];

        break;
    }
    case LOAD_STRING: {
        uint8_t reg;
        bs->Read(&reg);

        // get string length
        uint32_t len;
        bs->Read(&len);

        // read string based on length
        char *str = new char[len + 1];
        str[len] = '\0';

        bs->Read(str, len);

        // allocate heap value
        HeapValue *hv = m_state.HeapAlloc(thread);
        if (hv != nullptr) {
            hv->Assign(utf::Utf8String(str));

            // assign register value to the allocated object
            StackValue &sv = thread->m_regs[reg];
            sv.m_type = StackValue::HEAP_POINTER;
            sv.m_value.ptr = hv;
        }

        delete[] str;

        break;
    }
    case LOAD_ADDR: {
        uint8_t reg;
        bs->Read(&reg);

        uint32_t value;
        bs->Read(&value);

        StackValue &sv = thread->m_regs[reg];
        sv.m_type = StackValue::ADDRESS;
        sv.m_value.addr = value;

        break;
    }
    case LOAD_FUNC: {
        uint8_t reg;
        bs->Read(&reg);

        uint32_t addr;
        bs->Read(&addr);

        uint8_t nargs;
        bs->Read(&nargs);

        StackValue &sv = thread->m_regs[reg];
        sv.m_type = StackValue::FUNCTION;
        sv.m_value.func.m_addr = addr;
        sv.m_value.func.m_nargs = nargs;

        break;
    }
    case LOAD_TYPE: {
        uint8_t reg;
        bs->Read(&reg);

        uint16_t size;
        bs->Read(&size);

        ASSERT(size > 0);

        uint32_t *hashes = new uint32_t[size];

        // load (size) hashes.
        for (int i = 0; i < size; i++) {
            bs->Read(&hashes[i]);
        }

        // allocate heap value
        HeapValue *hv = m_state.HeapAlloc(thread);

        ASSERT(hv != nullptr);

        hv->Assign(TypeInfo(size, hashes));

        // assign register value to the allocated object
        StackValue &sv = thread->m_regs[reg];
        sv.m_type = StackValue::HEAP_POINTER;
        sv.m_value.ptr = hv;

        delete[] hashes;

        break;
    }
    case LOAD_MEM: {
        uint8_t dst;
        bs->Read(&dst);

        uint8_t src;
        bs->Read(&src);

        uint8_t idx;
        bs->Read(&idx);

        StackValue &sv = thread->m_regs[src];
        ASSERT(sv.m_type == StackValue::HEAP_POINTER);

        HeapValue *hv = sv.m_value.ptr;
        if (hv == nullptr) {
            m_state.ThrowException(thread, Exception::NullReferenceException());
        } else {
            Object *objptr = nullptr;
            if ((objptr = hv->GetPointer<Object>()) != nullptr) {
                ASSERT_MSG(idx < objptr->GetSize(), "member index out of bounds");
                thread->m_regs[dst] = objptr->GetMember(idx).value;
            } else {
                m_state.ThrowException(thread,
                    Exception(utf::Utf8String("not a standard object")));
            }
        }

        break;
    }
    case LOAD_MEM_HASH: {
        uint8_t dst;
        bs->Read(&dst);

        uint8_t src;
        bs->Read(&src);

        uint32_t hash;
        bs->Read(&hash);

        StackValue &sv = thread->m_regs[src];
        ASSERT(sv.m_type == StackValue::HEAP_POINTER);

        HeapValue *hv = sv.m_value.ptr;
        if (hv == nullptr) {
            m_state.ThrowException(thread, Exception::NullReferenceException());
        } else {
            Object *objptr = nullptr;
            if ((objptr = hv->GetPointer<Object>()) != nullptr) {
                Member *member = objptr->LookupMemberFromHash(hash);
                if (member == nullptr) {
                    m_state.ThrowException(thread, Exception::MemberNotFoundException());
                } else {
                    thread->m_regs[dst] = member->value;
                }
            } else {
                m_state.ThrowException(thread,
                    Exception(utf::Utf8String("not a standard object")));
            }
        }

        break;
    }
    case LOAD_ARRAYIDX: {
        uint8_t dst;
        bs->Read(&dst);

        uint8_t src;
        bs->Read(&src);

        uint8_t idx_reg;
        bs->Read(&idx_reg);

        StackValue &idx_sv = thread->m_regs[idx_reg];

        StackValue &sv = thread->m_regs[src];
        ASSERT_MSG(sv.m_type == StackValue::HEAP_POINTER, "source must be a pointer");

        HeapValue *hv = sv.m_value.ptr;

        utf::Utf8String *str_ptr = nullptr;

        if (hv == nullptr) {
            m_state.ThrowException(thread, Exception::NullReferenceException());
        } else {
            if (IS_VALUE_INTEGER(idx_sv)) {
                int64_t idx_i = GetIntFast(idx_sv);

                Array *arrayptr = nullptr;
                if ((arrayptr = hv->GetPointer<Array>()) != nullptr) {
                    if (idx_i >= arrayptr->GetSize()) {
                        m_state.ThrowException(thread, Exception::OutOfBoundsException());
                    } else {
                        thread->m_regs[dst] = arrayptr->AtIndex(idx_i);
                    }
                } else {
                    m_state.ThrowException(thread,
                        Exception(utf::Utf8String("object is not an array")));
                }
            } else if (IS_VALUE_STRING(idx_sv, str_ptr)) {
                m_state.ThrowException(thread,
                    Exception(utf::Utf8String("Map is not yet supported")));
            } else {
                m_state.ThrowException(thread,
                    Exception(utf::Utf8String("array index must be of type Int or String")));
            }
        }

        break;
    }
    case LOAD_NULL: {
        uint8_t reg;
        bs->Read(&reg);

        StackValue &sv = thread->m_regs[reg];
        sv.m_type = StackValue::HEAP_POINTER;
        sv.m_value.ptr = nullptr;

        break;
    }
    case LOAD_TRUE: {
        uint8_t reg;
        bs->Read(&reg);

        StackValue &sv = thread->m_regs[reg];
        sv.m_type = StackValue::BOOLEAN;
        sv.m_value.b = true;

        break;
    }
    case LOAD_FALSE: {
        uint8_t reg;
        bs->Read(&reg);

        StackValue &sv = thread->m_regs[reg];
        sv.m_type = StackValue::BOOLEAN;
        sv.m_value.b = false;

        break;
    }
    case MOV_OFFSET: {
        uint16_t offset;
        bs->Read(&offset);

        uint8_t reg;
        bs->Read(&reg);

        // copy value from register to stack value at (sp - offset)
        thread->m_stack[thread->m_stack.GetStackPointer() - offset] =
            thread->m_regs[reg];

        break;
    }
    case MOV_INDEX: {
        uint16_t idx;
        bs->Read(&idx);

        uint8_t reg;
        bs->Read(&reg);

        // copy value from register to stack value at index
        // NOTE: storing on main thread
        MAIN_THREAD->m_stack[idx] = thread->m_regs[reg];

        break;
    }
    case MOV_MEM: {
        uint8_t dst;
        bs->Read(&dst);

        uint8_t idx;
        bs->Read(&idx);

        uint8_t src;
        bs->Read(&src);

        StackValue &sv = thread->m_regs[dst];
        ASSERT_MSG(sv.m_type == StackValue::HEAP_POINTER, "destination must be a pointer");

        HeapValue *hv = sv.m_value.ptr;
        if (hv == nullptr) {
            m_state.ThrowException(thread, Exception::NullReferenceException());
        } else {
            Object *objptr = nullptr;
            if ((objptr = hv->GetPointer<Object>()) != nullptr) {
                ASSERT_MSG(idx < objptr->GetSize(), "member index out of bounds");
                objptr->GetMember(idx).value = thread->m_regs[src];
            } else {
                m_state.ThrowException(thread,
                    Exception(utf::Utf8String("not a standard object")));
            }
        }

        break;
    }
    case MOV_ARRAYIDX: {
        uint8_t dst;
        bs->Read(&dst);

        uint32_t idx;
        bs->Read(&idx);

        uint8_t src;
        bs->Read(&src);

        StackValue &sv = thread->m_regs[dst];
        ASSERT_MSG(sv.m_type == StackValue::HEAP_POINTER, "destination must be a pointer");

        HeapValue *hv = sv.m_value.ptr;
        if (hv == nullptr) {
            m_state.ThrowException(thread, Exception::NullReferenceException());
        } else {
            Array *arrayptr = nullptr;
            if ((arrayptr = hv->GetPointer<Array>()) != nullptr) {
                if (idx >= arrayptr->GetSize()) {
                    m_state.ThrowException(thread, Exception::OutOfBoundsException());
                } else {
                    arrayptr->AtIndex(idx) = thread->m_regs[src];
                }
            } else {
                m_state.ThrowException(thread,
                    Exception(utf::Utf8String("object is not an array")));
            }
        }

        break;
    }
    case MOV_REG: {
        uint8_t dst;
        bs->Read(&dst);

        uint8_t src;
        bs->Read(&src);

        thread->m_regs[dst] = thread->m_regs[src];

        break;
    }
    case HAS_MEM_HASH: {
        uint8_t dst;
        bs->Read(&dst);

        uint8_t src;
        bs->Read(&src);

        uint32_t hash;
        bs->Read(&hash);

        StackValue &sv = thread->m_regs[src];
        StackValue &res = thread->m_regs[dst];

        if (sv.m_type == StackValue::HEAP_POINTER) {
            HeapValue *hv = sv.m_value.ptr;
            if (hv != nullptr) {
                Object *objptr = nullptr;
                if ((objptr = hv->GetPointer<Object>()) != nullptr) {
                    Member *member = objptr->LookupMemberFromHash(hash);
                    if (member != nullptr) {
                        res = member->value;

                        // leave the statement
                        break;
                    }
                }
            }
        }

        // not found, set it to null
        res.m_type = StackValue::HEAP_POINTER;
        res.m_value.ptr = nullptr;

        break;
    }
    case PUSH: {
        uint8_t reg;
        bs->Read(&reg);

        // push a copy of the register value to the top of the stack
        thread->m_stack.Push(thread->m_regs[reg]);
        break;
    }
    case POP: {
        thread->m_stack.Pop();
        break;
    }
    case PUSH_ARRAY: {
        uint8_t dst;
        bs->Read(&dst);

        uint8_t src;
        bs->Read(&src);

        StackValue &sv = thread->m_regs[dst];
        ASSERT_MSG(sv.m_type == StackValue::HEAP_POINTER, "destination must be a pointer");

        HeapValue *hv = sv.m_value.ptr;
        if (hv == nullptr) {
            m_state.ThrowException(thread,
                Exception::NullReferenceException());
        } else {
            Array *arrayptr = nullptr;
            if ((arrayptr = hv->GetPointer<Array>()) != nullptr) {
                arrayptr->Push(thread->m_regs[src]);
            } else {
                m_state.ThrowException(thread,
                    Exception(utf::Utf8String("object is not an array")));
            }
        }

        break;
    }
    case ECHO: {
        uint8_t reg;
        bs->Read(&reg);

        // print out the value of the item in the register
        Print(thread->m_regs[reg]);
        
        break;
    }
    case ECHO_NEWLINE: {
        utf::fputs(UTF8_CSTR("\n"), stdout);
        break;
    }
    case JMP: {
        uint8_t reg;
        bs->Read(&reg);

        const StackValue &addr = thread->m_regs[reg];
        ASSERT_MSG(addr.m_type == StackValue::ADDRESS, "register must hold an address");

        bs->Seek(addr.m_value.addr);

        break;
    }
    case JE: {
        uint8_t reg;
        bs->Read(&reg);

        if (thread->m_regs.m_flags == EQUAL) {
            const StackValue &addr = thread->m_regs[reg];
            ASSERT_MSG(addr.m_type == StackValue::ADDRESS, "register must hold an address");

            bs->Seek(addr.m_value.addr);
        }

        break;
    }
    case JNE: {
        uint8_t reg;
        bs->Read(&reg);

        if (thread->m_regs.m_flags != EQUAL) {
            const StackValue &addr = thread->m_regs[reg];
            ASSERT_MSG(addr.m_type == StackValue::ADDRESS, "register must hold an address");

            bs->Seek(addr.m_value.addr);
        }

        break;
    }
    case JG: {
        uint8_t reg;
        bs->Read(&reg);

        if (thread->m_regs.m_flags == GREATER) {
            const StackValue &addr = thread->m_regs[reg];
            ASSERT_MSG(addr.m_type == StackValue::ADDRESS, "register must hold an address");

            bs->Seek(addr.m_value.addr);
        }

        break;
    }
    case JGE: {
        uint8_t reg;
        bs->Read(&reg);

        if (thread->m_regs.m_flags == GREATER || thread->m_regs.m_flags == EQUAL) {
            const StackValue &addr = thread->m_regs[reg];
            ASSERT_MSG(addr.m_type == StackValue::ADDRESS, "register must hold an address");

            bs->Seek(addr.m_value.addr);
        }

        break;
    }
    case CALL: {
        uint8_t reg;
        bs->Read(&reg);

        uint8_t num_args;
        bs->Read(&num_args);

        Invoke(thread, bs, thread->m_regs[reg], num_args);

        break;
    }
    case RET: {
        // get top of stack (should be the address before jumping)
        StackValue &top = thread->GetStack().Top();
        ASSERT(top.GetType() == StackValue::ADDRESS);
        
        // leave function and return to previous position
        bs->Seek(top.GetValue().addr);
        
        // pop from stack
        thread->GetStack().Pop();

        // decrease function depth
        thread->m_func_depth--;
        
        break;
    }
    case BEGIN_TRY: {
        // register that holds address of catch block
        uint8_t reg;
        bs->Read(&reg);

        // copy the value of the address for the catch-block
        const StackValue &catch_address = thread->m_regs[reg];
        ASSERT_MSG(catch_address.m_type == StackValue::ADDRESS, "register must hold an address");

        thread->m_exception_state.m_try_counter++;

        // increase stack size to store data about this try block
        StackValue info;
        info.m_type = StackValue::TRY_CATCH_INFO;
        info.m_value.try_catch_info.catch_address = catch_address.m_value.addr;

        // store the info
        thread->m_stack.Push(info);

        break;
    }
    case END_TRY: {
        // pop the try catch info from the stack
        ASSERT(thread->m_stack.Top().m_type == StackValue::TRY_CATCH_INFO);
        ASSERT(thread->m_exception_state.m_try_counter < 0);

        thread->m_stack.Pop();
        thread->m_exception_state.m_try_counter--;

        break;
    }
    case NEW: {
        uint8_t dst;
        bs->Read(&dst);

        uint8_t src;
        bs->Read(&src);

        // read value from register
        StackValue &type_sv = thread->m_regs[src];
        ASSERT(type_sv.m_type == StackValue::HEAP_POINTER);

        TypeInfo *type_ptr = type_sv.m_value.ptr->GetPointer<TypeInfo>();
        ASSERT(type_ptr != nullptr);

        // allocate heap object
        HeapValue *hv = m_state.HeapAlloc(thread);
        if (hv != nullptr) {
            // create the Object from the info type_ptr provides us with.
            hv->Assign(Object(type_ptr->GetSize(), type_ptr->GetHashes()));

            // assign register value to the allocated object
            StackValue &sv = thread->m_regs[dst];
            sv.m_type = StackValue::HEAP_POINTER;
            sv.m_value.ptr = hv;
        }

        break;
    }
    case NEW_ARRAY: {
        uint8_t dst;
        bs->Read(&dst);

        uint32_t size;
        bs->Read(&size);

        // allocate heap object
        HeapValue *hv = m_state.HeapAlloc(thread);
        if (hv != nullptr) {
            hv->Assign(Array(size));

            // assign register value to the allocated object
            StackValue &sv = thread->m_regs[dst];
            sv.m_type = StackValue::HEAP_POINTER;
            sv.m_value.ptr = hv;
        }

        break;
    }
    case CMP: {
        uint8_t lhs_reg;
        bs->Read(&lhs_reg);

        uint8_t rhs_reg;
        bs->Read(&rhs_reg);

        // load values from registers
        StackValue &lhs = thread->m_regs[lhs_reg];
        StackValue &rhs = thread->m_regs[rhs_reg];

        // COMPARE INTEGERS
        if (IS_VALUE_INTEGER(lhs) && IS_VALUE_INTEGER(rhs)) {
            int64_t left = GetValueInt64(thread, lhs);
            int64_t right = GetValueInt64(thread, rhs);

            if (left > right) {
                // set GREATER flag
                thread->m_regs.m_flags = GREATER;
            } else if (left == right) {
                // set EQUAL flag
                thread->m_regs.m_flags = EQUAL;
            } else {
                // set NONE flag
                thread->m_regs.m_flags = NONE;
            }
        // COMPARE BOOLEANS
        } else if (lhs.m_type == StackValue::BOOLEAN && rhs.m_type == StackValue::BOOLEAN) {
            bool left = lhs.m_value.b;
            bool right = rhs.m_value.b;

            if (left > right) {
                // set GREATER flag
                thread->m_regs.m_flags = GREATER;
            } else if (left == right) {
                // set EQUAL flag
                thread->m_regs.m_flags = EQUAL;
            } else {
                // set NONE flag
                thread->m_regs.m_flags = NONE;
            }
        } else if (lhs.m_type == StackValue::HEAP_POINTER) {
            COMPARE_REFERENCES(lhs, rhs);
        } else if (rhs.m_type == StackValue::HEAP_POINTER) {
            COMPARE_REFERENCES(rhs, lhs);
        } else if (lhs.m_type == StackValue::FUNCTION) {
            COMPARE_FUNCTIONS(lhs, rhs);
        } else if (rhs.m_type == StackValue::FUNCTION) {
            COMPARE_FUNCTIONS(rhs, lhs);
        // COMPARE FLOATING POINT
        } else if (IS_VALUE_FLOATING_POINT(lhs)) {
            COMPARE_FLOATING_POINT(lhs, rhs);
        } else if (IS_VALUE_FLOATING_POINT(rhs)) {
            COMPARE_FLOATING_POINT(rhs, lhs);
        } else {
            THROW_COMPARISON_ERROR(lhs, rhs);
        }

        break;
    }
    case CMPZ: {
        uint8_t lhs_reg;
        bs->Read(&lhs_reg);

        // load values from registers
        StackValue &lhs = thread->m_regs[lhs_reg];

        if (IS_VALUE_INTEGER(lhs)) {
            int64_t value = GetValueInt64(thread, lhs);

            if (value == 0) {
                // set EQUAL flag
                thread->m_regs.m_flags = EQUAL;
            } else {
                // set NONE flag
                thread->m_regs.m_flags = NONE;
            }
        } else if (IS_VALUE_FLOATING_POINT(lhs)) {
            double value = GetValueDouble(thread, lhs);

            if (value == 0.0) {
                // set EQUAL flag
                thread->m_regs.m_flags = EQUAL;
            } else {
                // set NONE flag
                thread->m_regs.m_flags = NONE;
            }
        } else if (lhs.m_type == StackValue::BOOLEAN) {
            if (!lhs.m_value.b) {
                // set EQUAL flag
                thread->m_regs.m_flags = EQUAL;
            } else {
                // set NONE flag
                thread->m_regs.m_flags = NONE;
            }
        } else if (lhs.m_type == StackValue::HEAP_POINTER) {
            if (lhs.m_value.ptr == nullptr) {
                // set EQUAL flag
                thread->m_regs.m_flags = EQUAL;
            } else {
                // set NONE flag
                thread->m_regs.m_flags = NONE;
            }
        } else if (lhs.m_type == StackValue::FUNCTION) {
            // set NONE flag
            thread->m_regs.m_flags = NONE;
        } else {
            char buffer[256];
            std::sprintf(buffer, "cannot determine if type '%s' is nonzero",
                lhs.GetTypeString());
            m_state.ThrowException(thread, Exception(utf::Utf8String(buffer)));
        }

        break;
    }
    case ADD: {
        uint8_t lhs_reg;
        bs->Read(&lhs_reg);

        uint8_t rhs_reg;
        bs->Read(&rhs_reg);

        uint8_t dst_reg;
        bs->Read(&dst_reg);

        // load values from registers
        StackValue &lhs = thread->m_regs[lhs_reg];
        StackValue &rhs = thread->m_regs[rhs_reg];

        StackValue result;
        result.m_type = MATCH_TYPES(lhs, rhs);

        if (IS_VALUE_INTEGER(lhs) && IS_VALUE_INTEGER(rhs)) {
            int64_t left = GetValueInt64(thread, lhs);
            int64_t right = GetValueInt64(thread, rhs);
            int64_t result_value = left + right;

            if (result.m_type == StackValue::INT32) {
                result.m_value.i32 = (int32_t)result_value;
            } else {
                result.m_value.i64 = result_value;
            }
        } else if (IS_VALUE_FLOATING_POINT(lhs) || IS_VALUE_FLOATING_POINT(rhs)) {
            double left = GetValueDouble(thread, lhs);
            double right = GetValueDouble(thread, rhs);
            double result_value = left + right;

            if (result.m_type == StackValue::FLOAT) {
                result.m_value.f = (float)result_value;
            } else {
                result.m_value.d = result_value;
            }
        } else {
            char buffer[256];
            std::sprintf(buffer, "cannot add types '%s' and '%s'",
                lhs.GetTypeString(), rhs.GetTypeString());
            m_state.ThrowException(thread, Exception(utf::Utf8String(buffer)));
        }

        // set the destination register to be the result
        thread->m_regs[dst_reg] = result;

        break;
    }
    case SUB: {
        uint8_t lhs_reg;
        bs->Read(&lhs_reg);

        uint8_t rhs_reg;
        bs->Read(&rhs_reg);

        uint8_t dst_reg;
        bs->Read(&dst_reg);

        // load values from registers
        StackValue &lhs = thread->m_regs[lhs_reg];
        StackValue &rhs = thread->m_regs[rhs_reg];

        StackValue result;
        result.m_type = MATCH_TYPES(lhs, rhs);

        if (lhs.m_type == StackValue::HEAP_POINTER) {
        } else if (IS_VALUE_INTEGER(lhs) && IS_VALUE_INTEGER(rhs)) {
            int64_t left = GetValueInt64(thread, lhs);
            int64_t right = GetValueInt64(thread, rhs);
            int64_t result_value = left - right;

            if (result.m_type == StackValue::INT32) {
                result.m_value.i32 = (int32_t)result_value;
            } else {
                result.m_value.i64 = result_value;
            }
        } else if (IS_VALUE_FLOATING_POINT(lhs) || IS_VALUE_FLOATING_POINT(rhs)) {
            double left = GetValueDouble(thread, lhs);
            double right = GetValueDouble(thread, rhs);
            double result_value = left - right;

            if (result.m_type == StackValue::FLOAT) {
                result.m_value.f = (float)result_value;
            } else {
                result.m_value.d = result_value;
            }
        } else {
            char buffer[256];
            std::sprintf(buffer, "cannot subtract types '%s' and '%s'",
                lhs.GetTypeString(), rhs.GetTypeString());
            m_state.ThrowException(thread, Exception(utf::Utf8String(buffer)));
        }

        // set the destination register to be the result
        thread->m_regs[dst_reg] = result;

        break;
    }
    case MUL: {
        uint8_t lhs_reg;
        bs->Read(&lhs_reg);

        uint8_t rhs_reg;
        bs->Read(&rhs_reg);

        uint8_t dst_reg;
        bs->Read(&dst_reg);

        // load values from registers
        StackValue &lhs = thread->m_regs[lhs_reg];
        StackValue &rhs = thread->m_regs[rhs_reg];

        StackValue result;
        result.m_type = MATCH_TYPES(lhs, rhs);

        if (lhs.m_type == StackValue::HEAP_POINTER) {
        } else if (IS_VALUE_INTEGER(lhs) && IS_VALUE_INTEGER(rhs)) {
            int64_t left = GetValueInt64(thread, lhs);
            int64_t right = GetValueInt64(thread, rhs);
            int64_t result_value = left * right;

            if (result.m_type == StackValue::INT32) {
                result.m_value.i32 = (int32_t)result_value;
            } else {
                result.m_value.i64 = result_value;
            }
        } else if (IS_VALUE_FLOATING_POINT(lhs) || IS_VALUE_FLOATING_POINT(rhs)) {
            double left = GetValueDouble(thread, lhs);
            double right = GetValueDouble(thread, rhs);
            double result_value = left * right;

            if (result.m_type == StackValue::FLOAT) {
                result.m_value.f = (float)result_value;
            } else {
                result.m_value.d = result_value;
            }
        } else {
            char buffer[256];
            std::sprintf(buffer, "cannot multiply types '%s' and '%s'",
                lhs.GetTypeString(), rhs.GetTypeString());
            m_state.ThrowException(thread, Exception(utf::Utf8String(buffer)));
        }

        // set the destination register to be the result
        thread->m_regs[dst_reg] = result;

        break;
    }
    case DIV: {
        uint8_t lhs_reg;
        bs->Read(&lhs_reg);

        uint8_t rhs_reg;
        bs->Read(&rhs_reg);

        uint8_t dst_reg;
        bs->Read(&dst_reg);

        // load values from registers
        StackValue &lhs = thread->m_regs[lhs_reg];
        StackValue &rhs = thread->m_regs[rhs_reg];

        StackValue result;
        result.m_type = MATCH_TYPES(lhs, rhs);

        if (lhs.m_type == StackValue::HEAP_POINTER) {
        } else if (IS_VALUE_INTEGER(lhs) && IS_VALUE_INTEGER(rhs)) {
            int64_t left = GetValueInt64(thread, lhs);
            int64_t right = GetValueInt64(thread, rhs);

            if (right == 0) {
                // division by zero
                m_state.ThrowException(thread, Exception::DivisionByZeroException());
            } else {
                int64_t result_value = left / right;

                if (result.m_type == StackValue::INT32) {
                    result.m_value.i32 = (int32_t)result_value;
                } else {
                    result.m_value.i64 = result_value;
                }
            }
        } else if (IS_VALUE_FLOATING_POINT(lhs) || IS_VALUE_FLOATING_POINT(rhs)) {
            double left = GetValueDouble(thread, lhs);
            double right = GetValueDouble(thread, rhs);

            if (right == 0.0) {
                // division by zero
                m_state.ThrowException(thread, Exception::DivisionByZeroException());
            } else {
                double result_value = left / right;

                if (result.m_type == StackValue::FLOAT) {
                    result.m_value.f = (float)result_value;
                } else {
                    result.m_value.d = result_value;
                }
            }
        } else {
            char buffer[256];
            std::sprintf(buffer, "cannot divide types '%s' and '%s'",
                lhs.GetTypeString(), rhs.GetTypeString());
            m_state.ThrowException(thread, Exception(utf::Utf8String(buffer)));
        }

        // set the destination register to be the result
        thread->m_regs[dst_reg] = result;

        break;
    }
    case NEG: {
        uint8_t reg;
        bs->Read(&reg);

        // load value from register
        StackValue &value = thread->m_regs[reg];

        if (IS_VALUE_INTEGER(value)) {
            int64_t i = GetValueInt64(thread, value);
            if (value.m_type == StackValue::INT32) {
                value.m_value.i32 = (int32_t)-i;
            } else {
                value.m_value.i64 = -i;
            }
        } else if (IS_VALUE_FLOATING_POINT(value)) {
            double d = GetValueDouble(thread, value);
            if (value.m_type == StackValue::FLOAT) {
                value.m_value.f = (float)-d;
            } else {
                value.m_value.d = -d;
            }
        } else {
            char buffer[256];
            std::sprintf(buffer, "cannot negate type '%s'", value.GetTypeString());
            m_state.ThrowException(thread, Exception(utf::Utf8String(buffer)));
        }

        break;
    }
    default: {
        int64_t last_pos = (int64_t)bs->Position() - sizeof(int8_t);
        utf::printf(UTF8_CSTR("unknown instruction '%d' referenced at location: 0x%" PRIx64 "\n"), code, last_pos);
        // seek to end of bytecode stream
        bs->Seek(bs->Size());
    }
    }

    // exception handling
    if (thread->m_exception_state.HasExceptionOccurred()) {
        ASSERT(thread->m_exception_state.m_try_counter < 0);

        // handle exception
        thread->m_exception_state.m_try_counter--;

        StackValue *top = NULL;
        while ((top = &thread->m_stack.Top())->m_type != StackValue::TRY_CATCH_INFO) {
            thread->m_stack.Pop();
        }

        // top should be exception data
        ASSERT(top != NULL && top->m_type == StackValue::TRY_CATCH_INFO);

        // jump to the catch block
        bs->Seek(top->m_value.try_catch_info.catch_address);
        // reset the exception flag
        thread->m_exception_state.m_exception_occured = false;

        // pop exception data from stack
        thread->m_stack.Pop();
    }
}

void VM::LaunchThread(ExecutionThread *thread)
{
    ASSERT(m_state.GetBytecodeStream() != nullptr);

    // create copy of the stream so we don't affect position change
    BytecodeStream bs = *m_state.GetBytecodeStream();

    while (!bs.Eof() && m_state.good) {
        uint8_t code;
        bs.Read(&code);

        HandleInstruction(thread, &bs, code);
    }
}

void VM::Execute()
{
    ASSERT(m_state.GetNumThreads() > 0);
    LaunchThread(MAIN_THREAD);
}
