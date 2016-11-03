#include <ace-vm/vm.hpp>
#include <ace-vm/stack_value.hpp>
#include <ace-vm/heap_value.hpp>
#include <ace-vm/object.hpp>

#include <common/instructions.hpp>
#include <common/utf8.hpp>

#include <iostream>
#include <algorithm>
#include <cstdio>
#include <cassert>

VM::VM(BytecodeStream *bs)
    : m_bs(bs),
      m_max_heap_objects(GC_THRESHOLD_MIN)
{
}

VM::~VM()
{
}

HeapValue *VM::HeapAlloc()
{
    int heap_size = m_heap.Size();
    if (heap_size >= GC_THRESHOLD_MAX) {
        // heap overflow.
        char buffer[256];
        std::sprintf(buffer, "heap overflow, heap size is %d", heap_size);
        ThrowException(Exception(buffer));
        return nullptr;
    } else if (heap_size >= m_max_heap_objects) {
        // run the gc
        MarkObjects(&m_exec_thread);
        m_heap.Sweep();
        /*utf::cout << "Garbage collection ran.\n";
        utf::cout << "\theap size before = " << heap_size << "\n";
        utf::cout << "\theap size now = " << m_heap.Size() << "\n";*/

        // check if size is still over the maximum,
        // and resize the maximum if necessary.
        if (m_heap.Size() >= m_max_heap_objects) {
            // resize max number of objects
            m_max_heap_objects = std::min(
                m_max_heap_objects * GC_THRESHOLD_MUL, GC_THRESHOLD_MAX);
        }
    }

    return m_heap.Alloc();
}

void VM::MarkObject(StackValue &object)
{
    if (object.m_type == StackValue::HEAP_POINTER && object.m_value.ptr != nullptr) {
        Object *obj_ptr = object.m_value.ptr->GetPointer<Object>();
        if (obj_ptr != nullptr) {
            int obj_size = obj_ptr->GetSize();
            for (int i = 0; i < obj_size; i++) {
                MarkObject(obj_ptr->GetMember(i));
            }
        }
        object.m_value.ptr->GetFlags() |= GC_MARKED;
    }
}

void VM::MarkObjects(ExecutionThread *thread)
{
    for (int i = thread->m_stack.GetStackPointer() - 1; i >= 0; i--) {
        MarkObject(thread->m_stack[i]);
    }
}

void VM::Echo(StackValue &value)
{
    // string buffer for printing datatype
    char str[256];

    switch (value.m_type) {
    case StackValue::INT32:
        utf::cout << value.m_value.i32;
        break;
    case StackValue::INT64:
        utf::cout << value.m_value.i64;
        break;
    case StackValue::FLOAT:
        utf::cout << value.m_value.f;
        break;
    case StackValue::DOUBLE:
        utf::cout << value.m_value.d;
        break;
    case StackValue::BOOLEAN:
        utf::cout << (value.m_value.b ? "true" : "false");
        break;
    case StackValue::HEAP_POINTER:
        if (value.m_value.ptr == nullptr) {
            // special case for null pointers
            utf::cout << "null";
        } else if (value.m_value.ptr->TypeCompatible<utf::Utf8String>()) {
            // print string value
            utf::cout << value.m_value.ptr->Get<utf::Utf8String>();
        } else {
            std::sprintf(str, "object<%p>", (void*)value.m_value.ptr);
            utf::cout << str;
        }

        break;
    case StackValue::FUNCTION:
        std::sprintf(str, "function<%du, %du>",
            value.m_value.func.m_addr, value.m_value.func.m_nargs);
        utf::cout << str;
        break;
    case StackValue::ADDRESS:
        std::sprintf(str, "address<%du>", value.m_value.addr);
        utf::cout << str;
        break;
    case StackValue::TYPE_INFO:
        std::sprintf(str, "type<%du>", value.m_value.type_info.m_size);
        utf::cout << str;
        break;
    }
}

void VM::InvokeFunction(StackValue &value, uint8_t num_args)
{
    if (value.m_type != StackValue::FUNCTION) {
        char buffer[256];
        std::sprintf(buffer, "cannot invoke type '%s' as a function",
            value.GetTypeString());
        ThrowException(Exception(buffer));
    } else if (value.m_value.func.m_nargs != num_args) {
        char buffer[256];
        std::sprintf(buffer, "expected %d parameters, received %d",
            (int)value.m_value.func.m_nargs, (int)num_args);
        ThrowException(Exception(buffer));
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

void VM::ThrowException(const Exception &exception)
{
    if (m_exec_thread.m_exception_state.m_try_counter > 0) {
        // exception can be handled
        m_exec_thread.m_exception_state.m_exception_occured = true;
    } else {
        // unhandled exception
        std::printf("unhandled exception: %s\n", exception.ToString().c_str());
        // seek to end of bytecode stream
        m_bs->Seek(m_bs->Size());
    }
}

void VM::HandleInstruction(uint8_t code)
{
    switch (code) {
    case STORE_STATIC_STRING:
    {
        // get string length
        uint32_t len;
        m_bs->Read(&len);

        // read string based on length
        char *str = new char[len + 1];
        m_bs->Read(str, len);
        str[len] = '\0';

        // the value will be freed on
        // the destructor call of m_static_memory
        HeapValue *hv = new HeapValue();
        hv->Assign(utf::Utf8String(str));

        StackValue sv;
        sv.m_type = StackValue::HEAP_POINTER;
        sv.m_value.ptr = hv;

        m_static_memory.Store(sv);

        delete[] str;

        break;
    }
    case STORE_STATIC_ADDRESS:
    {
        uint32_t value;
        m_bs->Read(&value);

        StackValue sv;
        sv.m_type = StackValue::ADDRESS;
        sv.m_value.addr = value;

        m_static_memory.Store(sv);

        break;
    }
    case STORE_STATIC_FUNCTION:
    {
        uint32_t addr;
        m_bs->Read(&addr);

        uint8_t nargs;
        m_bs->Read(&nargs);

        StackValue sv;
        sv.m_type = StackValue::FUNCTION;
        sv.m_value.func.m_addr = addr;
        sv.m_value.func.m_nargs = nargs;

        m_static_memory.Store(sv);

        break;
    }
    case STORE_STATIC_TYPE:
    {
        uint16_t size;
        m_bs->Read(&size);

        StackValue sv;
        sv.m_type = StackValue::TYPE_INFO;
        sv.m_value.type_info.m_size = size;

        m_static_memory.Store(sv);

        break;
    }
    case LOAD_I32:
    {
        uint8_t reg;
        m_bs->Read(&reg);

        // get register value given
        StackValue &value = m_exec_thread.m_regs[reg];
        value.m_type = StackValue::INT32;

        // read 32-bit integer into register value
        m_bs->Read(&value.m_value.i32);

        break;
    }
    case LOAD_I64:
    {
        uint8_t reg;
        m_bs->Read(&reg);

        // get register value given
        StackValue &value = m_exec_thread.m_regs[reg];
        value.m_type = StackValue::INT64;

        // read 64-bit integer into register value
        m_bs->Read(&value.m_value.i64);

        break;
    }
    case LOAD_F32:
    {
        uint8_t reg;
        m_bs->Read(&reg);

        // get register value given
        StackValue &value = m_exec_thread.m_regs[reg];
        value.m_type = StackValue::FLOAT;

        // read float into register value
        m_bs->Read(&value.m_value.f);

        break;
    }
    case LOAD_F64:
    {
        uint8_t reg;
        m_bs->Read(&reg);

        // get register value given
        StackValue &value = m_exec_thread.m_regs[reg];
        value.m_type = StackValue::DOUBLE;

        // read double into register value
        m_bs->Read(&value.m_value.d);

        break;
    }
    case LOAD_LOCAL:
    {
        uint8_t reg;
        m_bs->Read(&reg);

        uint16_t offset;
        m_bs->Read(&offset);

        // read value from stack at (sp - offset)
        // into the the register
        m_exec_thread.m_regs[reg] =
            m_exec_thread.m_stack[m_exec_thread.m_stack.GetStackPointer() - offset];

        break;
    }
    case LOAD_STATIC:
    {
        uint8_t reg;
        m_bs->Read(&reg);

        uint16_t index;
        m_bs->Read(&index);

        // read value from static memory
        // at the index into the the register
        m_exec_thread.m_regs[reg] = m_static_memory[index];

        break;
    }
    case LOAD_STRING:
    {
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
        HeapValue *hv = HeapAlloc();
        if (hv != nullptr) {
            hv->Assign(utf::Utf8String(str));

            // assign register value to the allocated object
            StackValue &sv = m_exec_thread.m_regs[reg];
            sv.m_type = StackValue::HEAP_POINTER;
            sv.m_value.ptr = hv;
        }

        delete[] str;

        break;
    }
    case LOAD_ADDR:
    {
        uint8_t reg;
        m_bs->Read(&reg);

        uint32_t value;
        m_bs->Read(&value);

        StackValue &sv = m_exec_thread.m_regs[reg];
        sv.m_type = StackValue::ADDRESS;
        sv.m_value.addr = value;

        break;
    }
    case LOAD_FUNC:
    {
        uint8_t reg;
        m_bs->Read(&reg);

        uint32_t addr;
        m_bs->Read(&addr);

        uint8_t nargs;
        m_bs->Read(&nargs);

        StackValue &sv = m_exec_thread.m_regs[reg];
        sv.m_type = StackValue::FUNCTION;
        sv.m_value.func.m_addr = addr;
        sv.m_value.func.m_nargs = nargs;

        break;
    }
    case LOAD_TYPE:
    {
        uint8_t reg;
        m_bs->Read(&reg);

        uint16_t size;
        m_bs->Read(&size);

        StackValue &sv = m_exec_thread.m_regs[reg];
        sv.m_type = StackValue::TYPE_INFO;
        sv.m_value.type_info.m_size = size;

        break;
    }
    case LOAD_MEM:
    {
        uint8_t dst;
        m_bs->Read(&dst);

        uint8_t src;
        m_bs->Read(&src);

        uint8_t idx;
        m_bs->Read(&idx);

        StackValue &sv = m_exec_thread.m_regs[src];
        assert(sv.m_type == StackValue::HEAP_POINTER && "source must be a pointer");

        HeapValue *hv = sv.m_value.ptr;
        if (hv == nullptr) {
            // null reference exception.
            ThrowException(Exception("attempted to access a member of a null object"));
        } else {
            Object *objptr = nullptr;
            if ((objptr = hv->GetPointer<Object>()) != nullptr) {
                assert(idx < objptr->GetSize() && "member index out of bounds");
                m_exec_thread.m_regs[dst] = objptr->GetMember(idx);
            } else {
                ThrowException(Exception("not a standard object"));
            }
        }

        break;
    }
    case LOAD_NULL:
    {
        uint8_t reg;
        m_bs->Read(&reg);

        StackValue &sv = m_exec_thread.m_regs[reg];
        sv.m_type = StackValue::HEAP_POINTER;
        sv.m_value.ptr = nullptr;

        break;
    }
    case LOAD_TRUE:
    {
        uint8_t reg;
        m_bs->Read(&reg);

        StackValue &sv = m_exec_thread.m_regs[reg];
        sv.m_type = StackValue::BOOLEAN;
        sv.m_value.b = true;

        break;
    }
    case LOAD_FALSE:
    {
        uint8_t reg;
        m_bs->Read(&reg);

        StackValue &sv = m_exec_thread.m_regs[reg];
        sv.m_type = StackValue::BOOLEAN;
        sv.m_value.b = false;

        break;
    }
    case MOV:
    {
        uint16_t offset;
        m_bs->Read(&offset);

        uint8_t reg;
        m_bs->Read(&reg);

        // copy value from register to stack value at (sp - offset)
        m_exec_thread.m_stack[m_exec_thread.m_stack.GetStackPointer() - offset] =
            m_exec_thread.m_regs[reg];

        break;
    }
    case MOV_MEM:
    {
        uint8_t dst;
        m_bs->Read(&dst);

        uint8_t idx;
        m_bs->Read(&idx);

        uint8_t src;
        m_bs->Read(&src);

        StackValue &sv = m_exec_thread.m_regs[dst];
        assert(sv.m_type == StackValue::HEAP_POINTER && "destination must be a pointer");

        HeapValue *hv = sv.m_value.ptr;
        if (hv == nullptr) {
            // null reference exception.
            ThrowException(Exception("attempted to store a member to a null object"));
        } else {
            Object *objptr = nullptr;
            if ((objptr = hv->GetPointer<Object>()) != nullptr) {
                assert(idx < objptr->GetSize() && "member index out of bounds");
                objptr->GetMember(idx) = m_exec_thread.m_regs[src];
            } else {
                ThrowException(Exception("not a standard object"));
            }
        }

        break;
    }
    case PUSH:
    {
        uint8_t reg;
        m_bs->Read(&reg);

        // push a copy of the register value to the top of the stack
        m_exec_thread.m_stack.Push(m_exec_thread.m_regs[reg]);

        break;
    }
    case POP:
    {
        m_exec_thread.m_stack.Pop();

        break;
    }
    case ECHO:
    {
        uint8_t reg;
        m_bs->Read(&reg);

        // print out the value of the item in the register
        Echo(m_exec_thread.m_regs[reg]);

        break;
    }
    case ECHO_NEWLINE:
    {
        utf::cout << "\n";

        break;
    }
    case JMP:
    {
        uint8_t reg;
        m_bs->Read(&reg);

        const StackValue &addr = m_exec_thread.m_regs[reg];
        assert(addr.m_type == StackValue::ADDRESS && "register must hold an address");

        m_bs->Seek(addr.m_value.addr);

        break;
    }
    case JE:
    {
        uint8_t reg;
        m_bs->Read(&reg);

        if (m_exec_thread.m_regs.m_flags == EQUAL) {
            const StackValue &addr = m_exec_thread.m_regs[reg];
            assert(addr.m_type == StackValue::ADDRESS && "register must hold an address");

            m_bs->Seek(addr.m_value.addr);
        }

        break;
    }
    case JNE:
    {
        uint8_t reg;
        m_bs->Read(&reg);

        if (m_exec_thread.m_regs.m_flags != EQUAL) {
            const StackValue &addr = m_exec_thread.m_regs[reg];
            assert(addr.m_type == StackValue::ADDRESS && "register must hold an address");

            m_bs->Seek(addr.m_value.addr);
        }

        break;
    }
    case JG:
    {
        uint8_t reg;
        m_bs->Read(&reg);

        if (m_exec_thread.m_regs.m_flags == GREATER) {
            const StackValue &addr = m_exec_thread.m_regs[reg];
            assert(addr.m_type == StackValue::ADDRESS && "register must hold an address");

            m_bs->Seek(addr.m_value.addr);
        }

        break;
    }
    case JGE:
    {
        uint8_t reg;
        m_bs->Read(&reg);

        if (m_exec_thread.m_regs.m_flags == GREATER || m_exec_thread.m_regs.m_flags == EQUAL) {
            const StackValue &addr = m_exec_thread.m_regs[reg];
            assert(addr.m_type == StackValue::ADDRESS && "register must hold an address");

            m_bs->Seek(addr.m_value.addr);
        }

        break;
    }
    case CALL:
    {
        uint8_t reg;
        m_bs->Read(&reg);

        uint8_t num_args;
        m_bs->Read(&num_args);

        InvokeFunction(m_exec_thread.m_regs[reg], num_args);

        break;
    }
    case BEGIN_TRY:
    {
        // register that holds address of catch block
        uint8_t reg;
        m_bs->Read(&reg);

        // copy the value of the address for the catch-block
        StackValue addr(m_exec_thread.m_regs[reg]);
        assert(addr.m_type == StackValue::ADDRESS && "register must hold an address");

        int try_counter_before = m_exec_thread.m_exception_state.m_try_counter++;
        // the size of the stack before, so we can revert to it on error
        int sp_before = m_exec_thread.m_stack.GetStackPointer();

        while (HasNextInstruction() &&
            m_exec_thread.m_exception_state.m_try_counter != try_counter_before) {

            // handle instructions until we reach the end of the block
            uint8_t code;
            m_bs->Read(&code, 1);

            HandleInstruction(code);

            if (m_exec_thread.m_exception_state.m_exception_occured) {
                // decrement the try counter
                m_exec_thread.m_exception_state.m_try_counter--;

                // pop all local variables from the stack
                while (sp_before < m_exec_thread.m_stack.GetStackPointer()) {
                    m_exec_thread.m_stack.Pop();
                }

                // jump to the catch block
                m_bs->Seek(addr.m_value.addr);
                // reset the exception flag
                m_exec_thread.m_exception_state.m_exception_occured = false;

                break;
            }
        }

        break;
    }
    case END_TRY:
    {
        m_exec_thread.m_exception_state.m_try_counter--;
        break;
    }
    case NEW:
    {
        uint8_t dst;
        m_bs->Read(&dst);

        uint8_t src;
        m_bs->Read(&src);

        // read value from register
        StackValue &type_sv = m_exec_thread.m_regs[src];
        assert(type_sv.m_type == StackValue::TYPE_INFO && "object must be type info");

        // get number of data members
        int size = type_sv.m_value.type_info.m_size;

        // allocate heap object
        HeapValue *hv = HeapAlloc();
        if (hv != nullptr) {
            hv->Assign(Object(size));

            // assign register value to the allocated object
            StackValue &sv = m_exec_thread.m_regs[dst];
            sv.m_type = StackValue::HEAP_POINTER;
            sv.m_value.ptr = hv;
        }

        break;
    }
    case CMP:
    {
        uint8_t lhs_reg;
        m_bs->Read(&lhs_reg);

        uint8_t rhs_reg;
        m_bs->Read(&rhs_reg);

        // load values from registers
        StackValue &lhs = m_exec_thread.m_regs[lhs_reg];
        StackValue &rhs = m_exec_thread.m_regs[rhs_reg];

        // COMPARE INTEGERS
        if (IS_VALUE_INTEGER(lhs) && IS_VALUE_INTEGER(rhs)) {
            int64_t left = GetValueInt64(lhs);
            int64_t right = GetValueInt64(rhs);

            if (left > right) {
                // set GREATER flag
                m_exec_thread.m_regs.m_flags = GREATER;
            } else if (left == right) {
                // set EQUAL flag
                m_exec_thread.m_regs.m_flags = EQUAL;
            } else {
                // set NONE flag
                m_exec_thread.m_regs.m_flags = NONE;
            }
        // COMPARE BOOLEANS
        } else if (lhs.m_type == StackValue::BOOLEAN && rhs.m_type == StackValue::BOOLEAN) {
            bool left = lhs.m_value.b;
            bool right = rhs.m_value.b;

            if (left > right) {
                // set GREATER flag
                m_exec_thread.m_regs.m_flags = GREATER;
            } else if (left == right) {
                // set EQUAL flag
                m_exec_thread.m_regs.m_flags = EQUAL;
            } else {
                // set NONE flag
                m_exec_thread.m_regs.m_flags = NONE;
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
    case CMPZ:
    {
        uint8_t lhs_reg;
        m_bs->Read(&lhs_reg);

        // load values from registers
        StackValue &lhs = m_exec_thread.m_regs[lhs_reg];

        if (IS_VALUE_INTEGER(lhs)) {
            int64_t value = GetValueInt64(lhs);

            if (value == 0) {
                // set EQUAL flag
                m_exec_thread.m_regs.m_flags = EQUAL;
            } else {
                // set NONE flag
                m_exec_thread.m_regs.m_flags = NONE;
            }
        } else if (IS_VALUE_FLOATING_POINT(lhs)) {
            double value = GetValueDouble(lhs);

            if (value == 0.0) {
                // set EQUAL flag
                m_exec_thread.m_regs.m_flags = EQUAL;
            } else {
                // set NONE flag
                m_exec_thread.m_regs.m_flags = NONE;
            }
        } else if (lhs.m_type == StackValue::BOOLEAN) {
            if (!lhs.m_value.b) {
                // set EQUAL flag
                m_exec_thread.m_regs.m_flags = EQUAL;
            } else {
                // set NONE flag
                m_exec_thread.m_regs.m_flags = NONE;
            }
        } else if (lhs.m_type == StackValue::HEAP_POINTER) {
            if (lhs.m_value.ptr == nullptr) {
                // set EQUAL flag
                m_exec_thread.m_regs.m_flags = EQUAL;
            } else {
                // set NONE flag
                m_exec_thread.m_regs.m_flags = NONE;
            }
        } else if (lhs.m_type == StackValue::FUNCTION) {
            // set NONE flag
            m_exec_thread.m_regs.m_flags = NONE;
        } else {
            char buffer[256];
            std::sprintf(buffer, "cannot determine if type '%s' is nonzero",
                lhs.GetTypeString());

            ThrowException(Exception(buffer));
        }

        break;
    }
    case ADD:
    {
        uint8_t lhs_reg;
        m_bs->Read(&lhs_reg);

        uint8_t rhs_reg;
        m_bs->Read(&rhs_reg);

        uint8_t dst_reg;
        m_bs->Read(&dst_reg);

        // load values from registers
        StackValue &lhs = m_exec_thread.m_regs[lhs_reg];
        StackValue &rhs = m_exec_thread.m_regs[rhs_reg];

        StackValue result;
        result.m_type = MATCH_TYPES(lhs, rhs);

        if (lhs.m_type == StackValue::HEAP_POINTER) {
            // TODO: Check for '__OPR_ADD__' function and call it
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

            ThrowException(Exception(buffer));
        }

        // set the desination register to be the result
        m_exec_thread.m_regs[dst_reg] = result;

        break;
    }
    case SUB:
    {
        uint8_t lhs_reg;
        m_bs->Read(&lhs_reg);

        uint8_t rhs_reg;
        m_bs->Read(&rhs_reg);

        uint8_t dst_reg;
        m_bs->Read(&dst_reg);

        // load values from registers
        StackValue &lhs = m_exec_thread.m_regs[lhs_reg];
        StackValue &rhs = m_exec_thread.m_regs[rhs_reg];

        StackValue result;
        result.m_type = MATCH_TYPES(lhs, rhs);

        if (lhs.m_type == StackValue::HEAP_POINTER) {
            // TODO: Check for '__OPR_SUB__' function and call it
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

            ThrowException(Exception(buffer));
        }

        // set the desination register to be the result
        m_exec_thread.m_regs[dst_reg] = result;

        break;
    }
    case MUL:
    {
        uint8_t lhs_reg;
        m_bs->Read(&lhs_reg);

        uint8_t rhs_reg;
        m_bs->Read(&rhs_reg);

        uint8_t dst_reg;
        m_bs->Read(&dst_reg);

        // load values from registers
        StackValue &lhs = m_exec_thread.m_regs[lhs_reg];
        StackValue &rhs = m_exec_thread.m_regs[rhs_reg];

        StackValue result;
        result.m_type = MATCH_TYPES(lhs, rhs);

        if (lhs.m_type == StackValue::HEAP_POINTER) {
            // TODO: Check for '__OPR_MUL__' function and call it
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

            ThrowException(Exception(buffer));
        }

        // set the desination register to be the result
        m_exec_thread.m_regs[dst_reg] = result;

        break;
    }
    case DIV:
    {
        uint8_t lhs_reg;
        m_bs->Read(&lhs_reg);

        uint8_t rhs_reg;
        m_bs->Read(&rhs_reg);

        uint8_t dst_reg;
        m_bs->Read(&dst_reg);

        // load values from registers
        StackValue &lhs = m_exec_thread.m_regs[lhs_reg];
        StackValue &rhs = m_exec_thread.m_regs[rhs_reg];

        StackValue result;
        result.m_type = MATCH_TYPES(lhs, rhs);

        if (lhs.m_type == StackValue::HEAP_POINTER) {
            // TODO: Check for '__OPR_DIV__' function and call it
        } else if (IS_VALUE_INTEGER(lhs) && IS_VALUE_INTEGER(rhs)) {
            int64_t left = GetValueInt64(lhs);
            int64_t right = GetValueInt64(rhs);

            if (right == 0) {
                // division by zero
                char buffer[256];
                std::sprintf(buffer, "attempted to divide '%d' by zero", (int)left);
                ThrowException(Exception(buffer));
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
                char buffer[256];
                std::sprintf(buffer, "attempted to divide '%f' by zero", left);
                ThrowException(Exception(buffer));
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

            ThrowException(Exception(buffer));
        }

        // set the desination register to be the result
        m_exec_thread.m_regs[dst_reg] = result;

        break;
    }
    default:
        std::printf("unknown instruction '%d' referenced at location: 0x%08x\n",
            (int)code, (int)m_bs->Position());

        // seek to end of bytecode stream
        m_bs->Seek(m_bs->Size());
    }
}

void VM::Execute()
{
    while (HasNextInstruction()) {
        uint8_t code;
        m_bs->Read(&code, 1);

        HandleInstruction(code);
    }
}
