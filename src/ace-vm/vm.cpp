#include <ace-vm/vm.hpp>
#include <ace-vm/stack_value.hpp>
#include <ace-vm/heap_value.hpp>
#include <ace-vm/array.hpp>
#include <ace-vm/object.hpp>

#include <common/instructions.hpp>
#include <common/utf8.hpp>

#include <algorithm>
#include <cstdio>
#include <cassert>
#include <cinttypes>

VM::VM(BytecodeStream *bs)
    : m_bs(bs)
{
}

VM::~VM()
{
}

void VM::PushNativeFunctionPtr(NativeFunctionPtr_t ptr)
{
    StackValue sv;
    sv.m_type = StackValue::NATIVE_FUNCTION;
    sv.m_value.native_func = ptr;
    m_state.m_exec_thread.m_stack.Push(sv);
}

void VM::Echo(StackValue &value)
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
            int sep_str_len = std::strlen(sep_str);

            int buffer_index = 1;
            const int buffer_size = 256;
            utf::Utf8String res("[", buffer_size);

            // convert all array elements to string
            const int size = arrayptr->GetSize();
            for (int i = 0; i < size; i++) {
                utf::Utf8String item_str = arrayptr->AtIndex(i).ToString();
                int len = item_str.GetLength();

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
            utf::printf(UTF8_CSTR("Object<%p>"), (void*)value.m_value.ptr);
        }

        break;
    }
    case StackValue::FUNCTION:
        utf::printf(UTF8_CSTR("Function<%du>"), value.m_value.func.m_addr);
        break;
    case StackValue::NATIVE_FUNCTION:
        utf::printf(UTF8_CSTR("NativeFunction<%p>"), (void*)value.m_value.native_func);
        break;
    case StackValue::ADDRESS:
        utf::printf(UTF8_CSTR("Address<%du>"), value.m_value.addr);
        break;
    case StackValue::TYPE_INFO:
        utf::printf(UTF8_CSTR("Type<%du>"), value.m_value.type_info.m_size);
        break;
    }
}

void VM::InvokeFunction(StackValue &value, uint8_t num_args)
{
    if (value.m_type != StackValue::FUNCTION) {
        if (value.m_type == StackValue::NATIVE_FUNCTION) {
            StackValue **args = new StackValue*[num_args];

            int i = m_state.m_exec_thread.m_stack.GetStackPointer() - 1;
            for (int j = 0; j < num_args && i >= 0; i--, j++) {
                args[j] = &m_state.m_exec_thread.m_stack[i];
            }

            value.m_value.native_func(&m_state, args, num_args);

            delete[] args;
        } else {
            char buffer[256];
            std::sprintf(buffer, "cannot invoke type '%s' as a native function",
                value.GetTypeString());
            m_state.ThrowException(Exception(buffer));
        }
    } else if (value.m_value.func.m_nargs != num_args) {
        m_state.ThrowException(
            Exception::InvalidArgsException(value.m_value.func.m_nargs, num_args));
    } else {
        // store current address
        uint32_t previous = m_bs->Position();
        // seek to the function's address
        m_bs->Seek(value.m_value.func.m_addr);

        while (m_bs->Position() < m_bs->Size()) {
            uint8_t code;
            m_bs->Read(&code, 1);

            if (code != RET) {
                HandleInstruction(code);
            } else {
                // leave function and return to previous position
                m_bs->Seek(previous);
                break;
            }
        }
    }
}

void VM::HandleInstruction(uint8_t code)
{
    switch (code) {
    case STORE_STATIC_STRING: {
        // get string length
        uint32_t len;
        m_bs->Read(&len);

        // read string based on length
        char *str = new char[len + 1];
        m_bs->Read(str, len);
        str[len] = '\0';

        // the value will be freed on
        // the destructor call of m_state.m_static_memory
        HeapValue *hv = new HeapValue();
        hv->Assign(utf::Utf8String(str));

        StackValue sv;
        sv.m_type = StackValue::HEAP_POINTER;
        sv.m_value.ptr = hv;

        m_state.m_static_memory.Store(sv);

        delete[] str;

        break;
    }
    case STORE_STATIC_ADDRESS: {
        uint32_t value;
        m_bs->Read(&value);

        StackValue sv;
        sv.m_type = StackValue::ADDRESS;
        sv.m_value.addr = value;

        m_state.m_static_memory.Store(sv);

        break;
    }
    case STORE_STATIC_FUNCTION: {
        uint32_t addr;
        m_bs->Read(&addr);

        uint8_t nargs;
        m_bs->Read(&nargs);

        StackValue sv;
        sv.m_type = StackValue::FUNCTION;
        sv.m_value.func.m_addr = addr;
        sv.m_value.func.m_nargs = nargs;

        m_state.m_static_memory.Store(sv);

        break;
    }
    case STORE_STATIC_TYPE: {
        uint16_t size;
        m_bs->Read(&size);

        StackValue sv;
        sv.m_type = StackValue::TYPE_INFO;
        sv.m_value.type_info.m_size = size;

        m_state.m_static_memory.Store(sv);

        break;
    }
    case LOAD_I32: {
        uint8_t reg;
        m_bs->Read(&reg);

        // get register value given
        StackValue &value = m_state.m_exec_thread.m_regs[reg];
        value.m_type = StackValue::INT32;

        // read 32-bit integer into register value
        m_bs->Read(&value.m_value.i32);

        break;
    }
    case LOAD_I64: {
        uint8_t reg;
        m_bs->Read(&reg);

        // get register value given
        StackValue &value = m_state.m_exec_thread.m_regs[reg];
        value.m_type = StackValue::INT64;

        // read 64-bit integer into register value
        m_bs->Read(&value.m_value.i64);

        break;
    }
    case LOAD_F32: {
        uint8_t reg;
        m_bs->Read(&reg);

        // get register value given
        StackValue &value = m_state.m_exec_thread.m_regs[reg];
        value.m_type = StackValue::FLOAT;

        // read float into register value
        m_bs->Read(&value.m_value.f);

        break;
    }
    case LOAD_F64: {
        uint8_t reg;
        m_bs->Read(&reg);

        // get register value given
        StackValue &value = m_state.m_exec_thread.m_regs[reg];
        value.m_type = StackValue::DOUBLE;

        // read double into register value
        m_bs->Read(&value.m_value.d);

        break;
    }
    case LOAD_OFFSET: {
        uint8_t reg;
        m_bs->Read(&reg);

        uint16_t offset;
        m_bs->Read(&offset);

        // read value from stack at (sp - offset)
        // into the the register
        m_state.m_exec_thread.m_regs[reg] =
            m_state.m_exec_thread.m_stack[m_state.m_exec_thread.m_stack.GetStackPointer() - offset];

        break;
    }
    case LOAD_INDEX: {
        uint8_t reg;
        m_bs->Read(&reg);

        uint16_t idx;
        m_bs->Read(&idx);

        // read value from stack at the index into the the register
        m_state.m_exec_thread.m_regs[reg] = m_state.m_exec_thread.m_stack[idx];

        break;
    }
    case LOAD_STATIC: {
        uint8_t reg;
        m_bs->Read(&reg);

        uint16_t index;
        m_bs->Read(&index);

        // read value from static memory
        // at the index into the the register
        m_state.m_exec_thread.m_regs[reg] = m_state.m_static_memory[index];

        break;
    }
    case LOAD_STRING: {
        uint8_t reg;
        m_bs->Read(&reg);

        // get string length
        uint32_t len;
        m_bs->Read(&len);

        // read string based on length
        char *str = new char[len + 1];
        m_bs->Read(str, len);
        str[len] = '\0';

        // allocate heap value
        HeapValue *hv = m_state.HeapAlloc();
        if (hv != nullptr) {
            hv->Assign(utf::Utf8String(str));

            // assign register value to the allocated object
            StackValue &sv = m_state.m_exec_thread.m_regs[reg];
            sv.m_type = StackValue::HEAP_POINTER;
            sv.m_value.ptr = hv;
        }

        delete[] str;

        break;
    }
    case LOAD_ADDR: {
        uint8_t reg;
        m_bs->Read(&reg);

        uint32_t value;
        m_bs->Read(&value);

        StackValue &sv = m_state.m_exec_thread.m_regs[reg];
        sv.m_type = StackValue::ADDRESS;
        sv.m_value.addr = value;

        break;
    }
    case LOAD_FUNC: {
        uint8_t reg;
        m_bs->Read(&reg);

        uint32_t addr;
        m_bs->Read(&addr);

        uint8_t nargs;
        m_bs->Read(&nargs);

        StackValue &sv = m_state.m_exec_thread.m_regs[reg];
        sv.m_type = StackValue::FUNCTION;
        sv.m_value.func.m_addr = addr;
        sv.m_value.func.m_nargs = nargs;

        break;
    }
    case LOAD_TYPE: {
        uint8_t reg;
        m_bs->Read(&reg);

        uint16_t size;
        m_bs->Read(&size);

        StackValue &sv = m_state.m_exec_thread.m_regs[reg];
        sv.m_type = StackValue::TYPE_INFO;
        sv.m_value.type_info.m_size = size;

        break;
    }
    case LOAD_MEM: {
        uint8_t dst;
        m_bs->Read(&dst);

        uint8_t src;
        m_bs->Read(&src);

        uint8_t idx;
        m_bs->Read(&idx);

        StackValue &sv = m_state.m_exec_thread.m_regs[src];
        assert(sv.m_type == StackValue::HEAP_POINTER && "source must be a pointer");

        HeapValue *hv = sv.m_value.ptr;
        if (hv == nullptr) {
            m_state.ThrowException(Exception::NullReferenceException());
        } else {
            Object *objptr = nullptr;
            if ((objptr = hv->GetPointer<Object>()) != nullptr) {
                assert(idx < objptr->GetSize() && "member index out of bounds");
                m_state.m_exec_thread.m_regs[dst] = objptr->GetMember(idx);
            } else {
                m_state.ThrowException(Exception(utf::Utf8String("not a standard object")));
            }
        }

        break;
    }
    case LOAD_ARRAYIDX: {
        uint8_t dst;
        m_bs->Read(&dst);

        uint8_t src;
        m_bs->Read(&src);

        uint32_t idx;
        m_bs->Read(&idx);

        StackValue &sv = m_state.m_exec_thread.m_regs[src];
        assert(sv.m_type == StackValue::HEAP_POINTER && "source must be a pointer");

        HeapValue *hv = sv.m_value.ptr;
        if (hv == nullptr) {
            m_state.ThrowException(Exception::NullReferenceException());
        } else {
            Array *arrayptr = nullptr;
            if ((arrayptr = hv->GetPointer<Array>()) != nullptr) {
                assert(idx < arrayptr->GetSize() && "index out of bounds of array");
                m_state.m_exec_thread.m_regs[dst] = arrayptr->AtIndex(idx);
            } else {
                m_state.ThrowException(Exception(utf::Utf8String("object is not an array")));
            }
        }

        break;
    }
    case LOAD_NULL: {
        uint8_t reg;
        m_bs->Read(&reg);

        StackValue &sv = m_state.m_exec_thread.m_regs[reg];
        sv.m_type = StackValue::HEAP_POINTER;
        sv.m_value.ptr = nullptr;

        break;
    }
    case LOAD_TRUE: {
        uint8_t reg;
        m_bs->Read(&reg);

        StackValue &sv = m_state.m_exec_thread.m_regs[reg];
        sv.m_type = StackValue::BOOLEAN;
        sv.m_value.b = true;

        break;
    }
    case LOAD_FALSE: {
        uint8_t reg;
        m_bs->Read(&reg);

        StackValue &sv = m_state.m_exec_thread.m_regs[reg];
        sv.m_type = StackValue::BOOLEAN;
        sv.m_value.b = false;

        break;
    }
    case MOV_OFFSET: {
        uint16_t offset;
        m_bs->Read(&offset);

        uint8_t reg;
        m_bs->Read(&reg);

        // copy value from register to stack value at (sp - offset)
        m_state.m_exec_thread.m_stack[m_state.m_exec_thread.m_stack.GetStackPointer() - offset] =
            m_state.m_exec_thread.m_regs[reg];

        break;
    }
    case MOV_INDEX: {
        uint16_t idx;
        m_bs->Read(&idx);

        uint8_t reg;
        m_bs->Read(&reg);

        // copy value from register to stack value at index
        m_state.m_exec_thread.m_stack[idx] = m_state.m_exec_thread.m_regs[reg];

        break;
    }
    case MOV_MEM: {
        uint8_t dst;
        m_bs->Read(&dst);

        uint8_t idx;
        m_bs->Read(&idx);

        uint8_t src;
        m_bs->Read(&src);

        StackValue &sv = m_state.m_exec_thread.m_regs[dst];
        assert(sv.m_type == StackValue::HEAP_POINTER && "destination must be a pointer");

        HeapValue *hv = sv.m_value.ptr;
        if (hv == nullptr) {
            m_state.ThrowException(Exception::NullReferenceException());
        } else {
            Object *objptr = nullptr;
            if ((objptr = hv->GetPointer<Object>()) != nullptr) {
                assert(idx < objptr->GetSize() && "member index out of bounds");
                objptr->GetMember(idx) = m_state.m_exec_thread.m_regs[src];
            } else {
                m_state.ThrowException(Exception(utf::Utf8String("not a standard object")));
            }
        }

        break;
    }
    case MOV_ARRAYIDX: {
        uint8_t dst;
        m_bs->Read(&dst);

        uint32_t idx;
        m_bs->Read(&idx);

        uint8_t src;
        m_bs->Read(&src);

        StackValue &sv = m_state.m_exec_thread.m_regs[dst];
        assert(sv.m_type == StackValue::HEAP_POINTER && "destination must be a pointer");

        HeapValue *hv = sv.m_value.ptr;
        if (hv == nullptr) {
            m_state.ThrowException(Exception::NullReferenceException());
        } else {
            Array *arrayptr = nullptr;
            if ((arrayptr = hv->GetPointer<Array>()) != nullptr) {
                assert(idx < arrayptr->GetSize() && "index out of bounds of array");
                arrayptr->AtIndex(idx) = m_state.m_exec_thread.m_regs[src];
            } else {
                m_state.ThrowException(Exception(utf::Utf8String("object is not an array")));
            }
        }

        break;
    }
    case MOV_REG: {
        uint8_t dst;
        m_bs->Read(&dst);

        uint8_t src;
        m_bs->Read(&src);

        m_state.m_exec_thread.m_regs[dst] = m_state.m_exec_thread.m_regs[src];

        break;
    }
    case PUSH: {
        uint8_t reg;
        m_bs->Read(&reg);

        // push a copy of the register value to the top of the stack
        m_state.m_exec_thread.m_stack.Push(m_state.m_exec_thread.m_regs[reg]);
        break;
    }
    case POP: {
        m_state.m_exec_thread.m_stack.Pop();
        break;
    }
    case PUSH_ARRAY: {
        uint8_t dst;
        m_bs->Read(&dst);

        uint8_t src;
        m_bs->Read(&src);

        StackValue &sv = m_state.m_exec_thread.m_regs[dst];
        assert(sv.m_type == StackValue::HEAP_POINTER && "destination must be a pointer");

        HeapValue *hv = sv.m_value.ptr;
        if (hv == nullptr) {
            m_state.ThrowException(Exception::NullReferenceException());
        } else {
            Array *arrayptr = nullptr;
            if ((arrayptr = hv->GetPointer<Array>()) != nullptr) {
                arrayptr->Push(m_state.m_exec_thread.m_regs[src]);
            } else {
                m_state.ThrowException(Exception(utf::Utf8String("object is not an array")));
            }
        }

        break;
    }
    case ECHO: {
        uint8_t reg;
        m_bs->Read(&reg);

        // print out the value of the item in the register
        Echo(m_state.m_exec_thread.m_regs[reg]);

        break;
    }
    case ECHO_NEWLINE: {
        utf::fputs(UTF8_CSTR("\n"), stdout);
        break;
    }
    case JMP: {
        uint8_t reg;
        m_bs->Read(&reg);

        const StackValue &addr = m_state.m_exec_thread.m_regs[reg];
        assert(addr.m_type == StackValue::ADDRESS && "register must hold an address");

        m_bs->Seek(addr.m_value.addr);

        break;
    }
    case JE: {
        uint8_t reg;
        m_bs->Read(&reg);

        if (m_state.m_exec_thread.m_regs.m_flags == EQUAL) {
            const StackValue &addr = m_state.m_exec_thread.m_regs[reg];
            assert(addr.m_type == StackValue::ADDRESS && "register must hold an address");

            m_bs->Seek(addr.m_value.addr);
        }

        break;
    }
    case JNE: {
        uint8_t reg;
        m_bs->Read(&reg);

        if (m_state.m_exec_thread.m_regs.m_flags != EQUAL) {
            const StackValue &addr = m_state.m_exec_thread.m_regs[reg];
            assert(addr.m_type == StackValue::ADDRESS && "register must hold an address");

            m_bs->Seek(addr.m_value.addr);
        }

        break;
    }
    case JG: {
        uint8_t reg;
        m_bs->Read(&reg);

        if (m_state.m_exec_thread.m_regs.m_flags == GREATER) {
            const StackValue &addr = m_state.m_exec_thread.m_regs[reg];
            assert(addr.m_type == StackValue::ADDRESS && "register must hold an address");

            m_bs->Seek(addr.m_value.addr);
        }

        break;
    }
    case JGE: {
        uint8_t reg;
        m_bs->Read(&reg);

        if (m_state.m_exec_thread.m_regs.m_flags == GREATER || m_state.m_exec_thread.m_regs.m_flags == EQUAL) {
            const StackValue &addr = m_state.m_exec_thread.m_regs[reg];
            assert(addr.m_type == StackValue::ADDRESS && "register must hold an address");

            m_bs->Seek(addr.m_value.addr);
        }

        break;
    }
    case CALL: {
        uint8_t reg;
        m_bs->Read(&reg);

        uint8_t num_args;
        m_bs->Read(&num_args);

        InvokeFunction(m_state.m_exec_thread.m_regs[reg], num_args);

        break;
    }
    case BEGIN_TRY: {
        // register that holds address of catch block
        uint8_t reg;
        m_bs->Read(&reg);

        // copy the value of the address for the catch-block
        StackValue addr(m_state.m_exec_thread.m_regs[reg]);
        assert(addr.m_type == StackValue::ADDRESS && "register must hold an address");

        int try_counter_before = m_state.m_exec_thread.m_exception_state.m_try_counter++;
        // the size of the stack before, so we can revert to it on error
        int sp_before = m_state.m_exec_thread.m_stack.GetStackPointer();

        while (HasNextInstruction() &&
            m_state.m_exec_thread.m_exception_state.m_try_counter != try_counter_before) {

            // handle instructions until we reach the end of the block
            uint8_t code;
            m_bs->Read(&code, 1);

            HandleInstruction(code);

            if (m_state.m_exec_thread.m_exception_state.m_exception_occured) {
                // decrement the try counter
                m_state.m_exec_thread.m_exception_state.m_try_counter--;

                // pop all local variables from the stack
                while (sp_before < m_state.m_exec_thread.m_stack.GetStackPointer()) {
                    m_state.m_exec_thread.m_stack.Pop();
                }

                // jump to the catch block
                m_bs->Seek(addr.m_value.addr);
                // reset the exception flag
                m_state.m_exec_thread.m_exception_state.m_exception_occured = false;

                break;
            }
        }

        break;
    }
    case END_TRY: {
        m_state.m_exec_thread.m_exception_state.m_try_counter--;
        break;
    }
    case NEW: {
        uint8_t dst;
        m_bs->Read(&dst);

        uint8_t src;
        m_bs->Read(&src);

        // read value from register
        StackValue &type_sv = m_state.m_exec_thread.m_regs[src];
        assert(type_sv.m_type == StackValue::TYPE_INFO && "object must be type info");

        // get number of data members
        int size = type_sv.m_value.type_info.m_size;

        // allocate heap object
        HeapValue *hv = m_state.HeapAlloc();
        if (hv != nullptr) {
            hv->Assign(Object(size));

            // assign register value to the allocated object
            StackValue &sv = m_state.m_exec_thread.m_regs[dst];
            sv.m_type = StackValue::HEAP_POINTER;
            sv.m_value.ptr = hv;
        }

        break;
    }
    case NEW_ARRAY: {
        uint8_t dst;
        m_bs->Read(&dst);

        uint32_t size;
        m_bs->Read(&size);

        // allocate heap object
        HeapValue *hv = m_state.HeapAlloc();
        if (hv != nullptr) {
            hv->Assign(Array(size));

            // assign register value to the allocated object
            StackValue &sv = m_state.m_exec_thread.m_regs[dst];
            sv.m_type = StackValue::HEAP_POINTER;
            sv.m_value.ptr = hv;
        }

        break;
    }
    case CMP: {
        uint8_t lhs_reg;
        m_bs->Read(&lhs_reg);

        uint8_t rhs_reg;
        m_bs->Read(&rhs_reg);

        // load values from registers
        StackValue &lhs = m_state.m_exec_thread.m_regs[lhs_reg];
        StackValue &rhs = m_state.m_exec_thread.m_regs[rhs_reg];

        // COMPARE INTEGERS
        if (IS_VALUE_INTEGER(lhs) && IS_VALUE_INTEGER(rhs)) {
            int64_t left = GetValueInt64(lhs);
            int64_t right = GetValueInt64(rhs);

            if (left > right) {
                // set GREATER flag
                m_state.m_exec_thread.m_regs.m_flags = GREATER;
            } else if (left == right) {
                // set EQUAL flag
                m_state.m_exec_thread.m_regs.m_flags = EQUAL;
            } else {
                // set NONE flag
                m_state.m_exec_thread.m_regs.m_flags = NONE;
            }
        // COMPARE BOOLEANS
        } else if (lhs.m_type == StackValue::BOOLEAN && rhs.m_type == StackValue::BOOLEAN) {
            bool left = lhs.m_value.b;
            bool right = rhs.m_value.b;

            if (left > right) {
                // set GREATER flag
                m_state.m_exec_thread.m_regs.m_flags = GREATER;
            } else if (left == right) {
                // set EQUAL flag
                m_state.m_exec_thread.m_regs.m_flags = EQUAL;
            } else {
                // set NONE flag
                m_state.m_exec_thread.m_regs.m_flags = NONE;
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
        m_bs->Read(&lhs_reg);

        // load values from registers
        StackValue &lhs = m_state.m_exec_thread.m_regs[lhs_reg];

        if (IS_VALUE_INTEGER(lhs)) {
            int64_t value = GetValueInt64(lhs);

            if (value == 0) {
                // set EQUAL flag
                m_state.m_exec_thread.m_regs.m_flags = EQUAL;
            } else {
                // set NONE flag
                m_state.m_exec_thread.m_regs.m_flags = NONE;
            }
        } else if (IS_VALUE_FLOATING_POINT(lhs)) {
            double value = GetValueDouble(lhs);

            if (value == 0.0) {
                // set EQUAL flag
                m_state.m_exec_thread.m_regs.m_flags = EQUAL;
            } else {
                // set NONE flag
                m_state.m_exec_thread.m_regs.m_flags = NONE;
            }
        } else if (lhs.m_type == StackValue::BOOLEAN) {
            if (!lhs.m_value.b) {
                // set EQUAL flag
                m_state.m_exec_thread.m_regs.m_flags = EQUAL;
            } else {
                // set NONE flag
                m_state.m_exec_thread.m_regs.m_flags = NONE;
            }
        } else if (lhs.m_type == StackValue::HEAP_POINTER) {
            if (lhs.m_value.ptr == nullptr) {
                // set EQUAL flag
                m_state.m_exec_thread.m_regs.m_flags = EQUAL;
            } else {
                // set NONE flag
                m_state.m_exec_thread.m_regs.m_flags = NONE;
            }
        } else if (lhs.m_type == StackValue::FUNCTION) {
            // set NONE flag
            m_state.m_exec_thread.m_regs.m_flags = NONE;
        } else {
            char buffer[256];
            std::sprintf(buffer, "cannot determine if type '%s' is nonzero",
                lhs.GetTypeString());
            m_state.ThrowException(Exception(utf::Utf8String(buffer)));
        }

        break;
    }
    case ADD: {
        uint8_t lhs_reg;
        m_bs->Read(&lhs_reg);

        uint8_t rhs_reg;
        m_bs->Read(&rhs_reg);

        uint8_t dst_reg;
        m_bs->Read(&dst_reg);

        // load values from registers
        StackValue &lhs = m_state.m_exec_thread.m_regs[lhs_reg];
        StackValue &rhs = m_state.m_exec_thread.m_regs[rhs_reg];

        StackValue result;
        result.m_type = MATCH_TYPES(lhs, rhs);

        if (lhs.m_type == StackValue::HEAP_POINTER) {
            
        } else if (IS_VALUE_INTEGER(lhs) && IS_VALUE_INTEGER(rhs)) {
            int64_t left = GetValueInt64(lhs);
            int64_t right = GetValueInt64(rhs);
            int64_t result_value = left + right;

            if (result.m_type == StackValue::INT32) {
                result.m_value.i32 = (int32_t)result_value;
            } else {
                result.m_value.i64 = result_value;
            }
        } else if (IS_VALUE_FLOATING_POINT(lhs) || IS_VALUE_FLOATING_POINT(rhs)) {
            double left = GetValueDouble(lhs);
            double right = GetValueDouble(rhs);
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
            m_state.ThrowException(Exception(utf::Utf8String(buffer)));
        }

        // set the desination register to be the result
        m_state.m_exec_thread.m_regs[dst_reg] = result;

        break;
    }
    case SUB: {
        uint8_t lhs_reg;
        m_bs->Read(&lhs_reg);

        uint8_t rhs_reg;
        m_bs->Read(&rhs_reg);

        uint8_t dst_reg;
        m_bs->Read(&dst_reg);

        // load values from registers
        StackValue &lhs = m_state.m_exec_thread.m_regs[lhs_reg];
        StackValue &rhs = m_state.m_exec_thread.m_regs[rhs_reg];

        StackValue result;
        result.m_type = MATCH_TYPES(lhs, rhs);

        if (lhs.m_type == StackValue::HEAP_POINTER) {
        } else if (IS_VALUE_INTEGER(lhs) && IS_VALUE_INTEGER(rhs)) {
            int64_t left = GetValueInt64(lhs);
            int64_t right = GetValueInt64(rhs);
            int64_t result_value = left - right;

            if (result.m_type == StackValue::INT32) {
                result.m_value.i32 = (int32_t)result_value;
            } else {
                result.m_value.i64 = result_value;
            }
        } else if (IS_VALUE_FLOATING_POINT(lhs) || IS_VALUE_FLOATING_POINT(rhs)) {
            double left = GetValueDouble(lhs);
            double right = GetValueDouble(rhs);
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
            m_state.ThrowException(Exception(utf::Utf8String(buffer)));
        }

        // set the desination register to be the result
        m_state.m_exec_thread.m_regs[dst_reg] = result;

        break;
    }
    case MUL: {
        uint8_t lhs_reg;
        m_bs->Read(&lhs_reg);

        uint8_t rhs_reg;
        m_bs->Read(&rhs_reg);

        uint8_t dst_reg;
        m_bs->Read(&dst_reg);

        // load values from registers
        StackValue &lhs = m_state.m_exec_thread.m_regs[lhs_reg];
        StackValue &rhs = m_state.m_exec_thread.m_regs[rhs_reg];

        StackValue result;
        result.m_type = MATCH_TYPES(lhs, rhs);

        if (lhs.m_type == StackValue::HEAP_POINTER) {
        } else if (IS_VALUE_INTEGER(lhs) && IS_VALUE_INTEGER(rhs)) {
            int64_t left = GetValueInt64(lhs);
            int64_t right = GetValueInt64(rhs);
            int64_t result_value = left * right;

            if (result.m_type == StackValue::INT32) {
                result.m_value.i32 = (int32_t)result_value;
            } else {
                result.m_value.i64 = result_value;
            }
        } else if (IS_VALUE_FLOATING_POINT(lhs) || IS_VALUE_FLOATING_POINT(rhs)) {
            double left = GetValueDouble(lhs);
            double right = GetValueDouble(rhs);
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
            m_state.ThrowException(Exception(utf::Utf8String(buffer)));
        }

        // set the desination register to be the result
        m_state.m_exec_thread.m_regs[dst_reg] = result;

        break;
    }
    case DIV: {
        uint8_t lhs_reg;
        m_bs->Read(&lhs_reg);

        uint8_t rhs_reg;
        m_bs->Read(&rhs_reg);

        uint8_t dst_reg;
        m_bs->Read(&dst_reg);

        // load values from registers
        StackValue &lhs = m_state.m_exec_thread.m_regs[lhs_reg];
        StackValue &rhs = m_state.m_exec_thread.m_regs[rhs_reg];

        StackValue result;
        result.m_type = MATCH_TYPES(lhs, rhs);

        if (lhs.m_type == StackValue::HEAP_POINTER) {
        } else if (IS_VALUE_INTEGER(lhs) && IS_VALUE_INTEGER(rhs)) {
            int64_t left = GetValueInt64(lhs);
            int64_t right = GetValueInt64(rhs);

            if (right == 0) {
                // division by zero
                m_state.ThrowException(Exception::DivisionByZeroException());
            } else {
                int64_t result_value = left / right;

                if (result.m_type == StackValue::INT32) {
                    result.m_value.i32 = (int32_t)result_value;
                } else {
                    result.m_value.i64 = result_value;
                }
            }
        } else if (IS_VALUE_FLOATING_POINT(lhs) || IS_VALUE_FLOATING_POINT(rhs)) {
            double left = GetValueDouble(lhs);
            double right = GetValueDouble(rhs);

            if (right == 0.0) {
                // division by zero
                m_state.ThrowException(Exception::DivisionByZeroException());
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
            m_state.ThrowException(Exception(utf::Utf8String(buffer)));
        }

        // set the desination register to be the result
        m_state.m_exec_thread.m_regs[dst_reg] = result;

        break;
    }
    case NEG: {
        uint8_t reg;
        m_bs->Read(&reg);

        // load value from register
        StackValue &value = m_state.m_exec_thread.m_regs[reg];

        if (IS_VALUE_INTEGER(value)) {
            int64_t i = GetValueInt64(value);
            if (value.m_type == StackValue::INT32) {
                value.m_value.i32 = (int32_t)-i;
            } else {
                value.m_value.i64 = -i;
            }
        } else if (IS_VALUE_FLOATING_POINT(value)) {
            double d = GetValueDouble(value);
            if (value.m_type == StackValue::FLOAT) {
                value.m_value.f = (float)-d;
            } else {
                value.m_value.d = -d;
            }
        } else {
            char buffer[256];
            std::sprintf(buffer, "cannot negate type '%s'", value.GetTypeString());
            m_state.ThrowException(Exception(utf::Utf8String(buffer)));
        }

        break;
    }
    default:
        utf::printf(UTF8_CSTR("unknown instruction '%d' referenced at location: 0x%08x\n"),
            (int)code, (int)m_bs->Position());
        // seek to end of bytecode stream
        m_bs->Seek(m_bs->Size());
    }
}

void VM::Execute()
{
    if (!m_state.good) {
        m_state.ThrowException(Exception("VM is in exception state, cannot continue"));
    }
    while (HasNextInstruction() && m_state.good) {
        uint8_t code;
        m_bs->Read(&code, 1);

        HandleInstruction(code);
    }
}
