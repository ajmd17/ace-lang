#include <ace-vm/VM.hpp>
#include <ace-vm/Value.hpp>
#include <ace-vm/HeapValue.hpp>
#include <ace-vm/Array.hpp>
#include <ace-vm/Object.hpp>
#include <ace-vm/TypeInfo.hpp>

#include <common/typedefs.hpp>
#include <common/instructions.hpp>
#include <common/utf8.hpp>
#include <common/my_assert.hpp>

#include <algorithm>
#include <cstdio>
#include <cinttypes>
#include <mutex>

namespace ace {
namespace vm {

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

    Value sv;
    sv.m_type = Value::NATIVE_FUNCTION;
    sv.m_value.native_func = ptr;
    MAIN_THREAD->m_stack.Push(sv);
}

void VM::Print(const Value &value)
{
    switch (value.m_type) {
        case Value::I32:
            utf::printf(UTF8_CSTR("%d"), value.m_value.i32);
            break;
        case Value::I64:
            utf::printf(UTF8_CSTR("%" PRId64), value.m_value.i64);
            break;
        case Value::F32:
            utf::printf(UTF8_CSTR("%g"), value.m_value.f);
            break;
        case Value::F64:
            utf::printf(UTF8_CSTR("%g"), value.m_value.d);
            break;
        case Value::BOOLEAN:
            utf::fputs(value.m_value.b ? UTF8_CSTR("true") : UTF8_CSTR("false"), stdout);
            break;
        case Value::HEAP_POINTER: {
            if (!value.m_value.ptr) {
                // special case for null pointers
                utf::fputs(UTF8_CSTR("nil"), stdout);
            } else if (utf::Utf8String *str = value.m_value.ptr->GetPointer<utf::Utf8String>()) {
                // print string value
                utf::cout << *str;
            } else if (Array *arrayptr = value.m_value.ptr->GetPointer<Array>()) {
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
                utf::cout << value.ToString();
            }

            break;
        }
        case Value::FUNCTION: utf::fputs(UTF8_CSTR("Function"), stdout); break;
        case Value::NATIVE_FUNCTION: utf::fputs(UTF8_CSTR("NativeFunction"), stdout); break;
        default: utf::fputs(UTF8_CSTR("??"), stdout); break;
    }
}

void VM::Invoke(ExecutionThread *thread, BytecodeStream *bs, const Value &value, uint8_t num_args)
{
    if (value.m_type != Value::FUNCTION) {
        if (value.m_type == Value::NATIVE_FUNCTION) {
            Value **args = new Value*[num_args > 0 ? num_args : 1];

            int i = (int)thread->m_stack.GetStackPointer() - 1;
            for (int j = num_args - 1; j >= 0 && i >= 0; i--, j--) {
                args[j] = &thread->m_stack[i];
            }

            value.m_value.native_func(ace::sdk::Params { &m_state, thread, bs, args, num_args });

            delete[] args;
        } else {
            char buffer[256];
            std::sprintf(buffer, "cannot invoke type '%s' as a function",
                value.GetTypeString());
            m_state.ThrowException(thread, Exception(buffer));
        }
    } else if (value.m_value.func.m_is_variadic && num_args < value.m_value.func.m_nargs - 1) {
        // if variadic, make sure the arg count is /at least/ what is required
        m_state.ThrowException(thread,
            Exception::InvalidArgsException(value.m_value.func.m_nargs, num_args, true));
    } else if (!value.m_value.func.m_is_variadic && value.m_value.func.m_nargs != num_args) {
        m_state.ThrowException(thread,
            Exception::InvalidArgsException(value.m_value.func.m_nargs, num_args));
    } else {
        Value previous_addr;
        previous_addr.m_type = Value::FUNCTION_CALL;
        previous_addr.m_value.call.varargs_push = 0;
        // store current address
        previous_addr.m_value.call.addr = (uint32_t)bs->Position();

        if (value.m_value.func.m_is_variadic) {
            // for each argument that is over the expected size, we must pop it from
            // the stack and add it to a new array.
            int varargs_amt = num_args - value.m_value.func.m_nargs + 1;
            if (varargs_amt < 0) {
                varargs_amt = 0;
            }
            
            // set varargs_push value so we know how to get back to the stack size before.
            previous_addr.m_value.call.varargs_push = varargs_amt - 1;
            
            // allocate heap object
            HeapValue *hv = m_state.HeapAlloc(thread);
            ASSERT(hv != nullptr);

            // create Array object to hold variadic args
            Array arr(varargs_amt);

            for (int i = varargs_amt - 1; i >= 0; i--) {
                // push to array
                arr.AtIndex(i, thread->GetStack().Top());
                thread->GetStack().Pop();
            }

            // assign heap value to our array
            hv->Assign(arr);

            Value array_value;
            array_value.m_type = Value::HEAP_POINTER;
            array_value.m_value.ptr = hv;

            // push the array to the stack
            thread->GetStack().Push(array_value);
        }

        // push the address
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
    
    if (thread->m_exception_state.HasExceptionOccurred()) {
        if (thread->m_exception_state.m_try_counter < 0) {
            // handle exception
            thread->m_exception_state.m_try_counter--;

            Value *top = nullptr;
            while ((top = &thread->m_stack.Top())->m_type != Value::TRY_CATCH_INFO) {
                thread->m_stack.Pop();
            }

            // top should be exception data
            ASSERT(top != nullptr && top->m_type == Value::TRY_CATCH_INFO);

            // jump to the catch block
            bs->Seek(top->m_value.try_catch_info.catch_address);
            // reset the exception flag
            thread->m_exception_state.m_exception_occured = false;

            // pop exception data from stack
            thread->m_stack.Pop();
        }

        return;
    }

    switch (code) {
    case STORE_STATIC_STRING: {
        // get string length
        uint32_t len; bs->Read(&len);

        // read string based on length
        char *str = new char[len + 1];
        bs->Read(str, len);
        str[len] = '\0';

        // the value will be freed on
        // the destructor call of m_state.m_static_memory
        HeapValue *hv = new HeapValue();
        hv->Assign(utf::Utf8String(str));

        Value sv;
        sv.m_type = Value::HEAP_POINTER;
        sv.m_value.ptr = hv;

        m_state.m_static_memory.Store(std::move(sv));

        delete[] str;

        break;
    }
    case STORE_STATIC_ADDRESS: {
        uint32_t value; bs->Read(&value);

        Value sv;
        sv.m_type = Value::ADDRESS;
        sv.m_value.addr = value;

        m_state.m_static_memory.Store(std::move(sv));

        break;
    }
    case STORE_STATIC_FUNCTION: {
        uint32_t addr; bs->Read(&addr);
        uint8_t nargs; bs->Read(&nargs);
        uint8_t is_variadic; bs->Read(&is_variadic);

        Value sv;
        sv.m_type = Value::FUNCTION;
        sv.m_value.func.m_addr = addr;
        sv.m_value.func.m_nargs = nargs;
        sv.m_value.func.m_is_variadic = is_variadic;

        m_state.m_static_memory.Store(std::move(sv));

        break;
    }
    case STORE_STATIC_TYPE: {
        uint16_t type_name_len; bs->Read(&type_name_len);

        char *type_name = new char[type_name_len + 1];
        type_name[type_name_len] = '\0';
        bs->Read(type_name, type_name_len);

        uint16_t size; bs->Read(&size);

        ASSERT(size > 0);

        char **names = new char*[size];
        // load each name
        for (int i = 0; i < size; i++) {
            uint16_t length;
            bs->Read(&length);

            names[i] = new char[length + 1];
            names[i][length] = '\0';
            bs->Read(names[i], length);
        }

        // the value will be freed on
        // the destructor call of m_state.m_static_memory
        HeapValue *hv = new HeapValue();
        hv->Assign(TypeInfo(type_name, size, names));

        Value sv;
        sv.m_type = Value::HEAP_POINTER;
        sv.m_value.ptr = hv;
        m_state.m_static_memory.Store(std::move(sv));

        delete[] type_name;
        
        // delete the names
        for (size_t i = 0; i < size; i++) {
            delete[] names[i];
        }
        delete[] names;

        break;
    }
    case LOAD_I32: {
        uint8_t reg; bs->Read(&reg);
        aint32 i32; bs->Read(&i32);

        // get register value given
        Value &value = thread->m_regs[reg];
        value.m_type = Value::I32;
        value.m_value.i32 = i32;

        break;
    }
    case LOAD_I64: {
        uint8_t reg; bs->Read(&reg);
        aint64 i64; bs->Read(&i64);

        // get register value given
        Value &value = thread->m_regs[reg];
        value.m_type = Value::I64;
        value.m_value.i64 = i64;

        break;
    }
    case LOAD_F32: {
        uint8_t reg; bs->Read(&reg);
        afloat32 f; bs->Read(&f);

        // get register value given
        Value &value = thread->m_regs[reg];
        value.m_type = Value::F32;
        value.m_value.f = f;

        break;
    }
    case LOAD_F64: {
        uint8_t reg; bs->Read(&reg);
        afloat64 d; bs->Read(&d);

        // get register value given
        Value &value = thread->m_regs[reg];
        value.m_type = Value::F64;
        value.m_value.d = d;

        break;
    }
    case LOAD_OFFSET: {
        uint8_t reg; bs->Read(&reg);
        uint16_t offset; bs->Read(&offset);

        // read value from stack at (sp - offset)
        // into the the register
        thread->m_regs[reg] = thread->m_stack[thread->m_stack.GetStackPointer() - offset];

        break;
    }
    case LOAD_INDEX: {
        uint8_t reg; bs->Read(&reg);
        uint16_t idx; bs->Read(&idx);

        // read value from stack at the index into the the register
        // NOTE: read from main thread
        thread->m_regs[reg] = MAIN_THREAD->m_stack[idx];

        break;
    }
    case LOAD_STATIC: {
        uint8_t reg; bs->Read(&reg);
        uint16_t index; bs->Read(&index);

        // read value from static memory
        // at the index into the the register
        thread->m_regs[reg] = m_state.m_static_memory[index];

        break;
    }
    case LOAD_STRING: {
        uint8_t reg; bs->Read(&reg);
        // get string length
        uint32_t len; bs->Read(&len);

        // read string based on length
        char *str = new char[len + 1];
        str[len] = '\0';
        bs->Read(str, len);

        // allocate heap value
        HeapValue *hv = m_state.HeapAlloc(thread);
        if (hv != nullptr) {
            hv->Assign(utf::Utf8String(str));

            // assign register value to the allocated object
            Value &sv = thread->m_regs[reg];
            sv.m_type = Value::HEAP_POINTER;
            sv.m_value.ptr = hv;
        }

        delete[] str;

        break;
    }
    case LOAD_ADDR: {
        uint8_t reg; bs->Read(&reg);
        uint32_t value; bs->Read(&value);

        Value &sv = thread->m_regs[reg];
        sv.m_type = Value::ADDRESS;
        sv.m_value.addr = value;

        break;
    }
    case LOAD_FUNC: {
        uint8_t reg; bs->Read(&reg);
        uint32_t addr; bs->Read(&addr);
        uint8_t nargs; bs->Read(&nargs);
        uint8_t is_variadic; bs->Read(&is_variadic);

        Value &sv = thread->m_regs[reg];
        sv.m_type = Value::FUNCTION;
        sv.m_value.func.m_addr = addr;
        sv.m_value.func.m_nargs = nargs;
        sv.m_value.func.m_is_variadic = is_variadic;

        break;
    }
    case LOAD_TYPE: {
        uint8_t reg; bs->Read(&reg);

        uint16_t type_name_len; bs->Read(&type_name_len);

        char *type_name = new char[type_name_len + 1];
        type_name[type_name_len] = '\0';
        bs->Read(type_name, type_name_len);

        // number of members
        uint16_t size; bs->Read(&size);
        ASSERT(size > 0);

        char **names = new char*[size];
        // load each name
        for (int i = 0; i < size; i++) {
            uint16_t length;
            bs->Read(&length);

            names[i] = new char[length + 1];
            names[i][length] = '\0';
            bs->Read(names[i], length);
        }

        // allocate heap value
        HeapValue *hv = m_state.HeapAlloc(thread);
        ASSERT(hv != nullptr);

        hv->Assign(TypeInfo(type_name, size, names));

        // assign register value to the allocated object
        Value &sv = thread->m_regs[reg];
        sv.m_type = Value::HEAP_POINTER;
        sv.m_value.ptr = hv;

        delete[] type_name;
        
        // delete the names
        for (size_t i = 0; i < size; i++) {
            delete[] names[i];
        }
        delete[] names;

        break;
    }
    case LOAD_MEM: {
        uint8_t dst; bs->Read(&dst);
        uint8_t src; bs->Read(&src);
        uint8_t idx; bs->Read(&idx);

        Value &sv = thread->m_regs[src];
        ASSERT(sv.m_type == Value::HEAP_POINTER);

        HeapValue *hv = sv.m_value.ptr;
        if (hv == nullptr) {
            m_state.ThrowException(thread, Exception::NullReferenceException());
        } else {
            if (Object *objptr = hv->GetPointer<Object>()) {
                const vm::TypeInfo *type_ptr = objptr->GetTypePtr();
                
                ASSERT(type_ptr != nullptr);
                ASSERT_MSG(idx < type_ptr->GetSize(), "member index out of bounds");
                
                thread->m_regs[dst] = objptr->GetMember(idx).value;
            } else {
                m_state.ThrowException(thread,
                    Exception(utf::Utf8String("not a standard object")));
            }
        }

        break;
    }
    case LOAD_MEM_HASH: {
        uint8_t dst; bs->Read(&dst);
        uint8_t src; bs->Read(&src);
        uint32_t hash; bs->Read(&hash);

        Value &sv = thread->m_regs[src];
        ASSERT(sv.m_type == Value::HEAP_POINTER);

        HeapValue *hv = sv.m_value.ptr;
        if (hv == nullptr) {
            m_state.ThrowException(thread, Exception::NullReferenceException());
        } else {
            if (Object *objptr = hv->GetPointer<Object>()) {
                if (Member *member = objptr->LookupMemberFromHash(hash)) {
                    thread->m_regs[dst] = member->value;
                } else {
                    m_state.ThrowException(thread, Exception::MemberNotFoundException());
                }
            } else {
                m_state.ThrowException(thread,
                    Exception(utf::Utf8String("not a standard object")));
            }
        }

        break;
    }
    case LOAD_ARRAYIDX: {
        uint8_t dst; bs->Read(&dst);
        uint8_t src; bs->Read(&src);
        uint8_t idx_reg; bs->Read(&idx_reg);

        Value &sv = thread->m_regs[src];
        ASSERT_MSG(sv.m_type == Value::HEAP_POINTER, "source must be a pointer");

        Value &idx_sv = thread->m_regs[idx_reg];

        if (HeapValue *hv = sv.m_value.ptr) {
            union {
                aint64 index;
                utf::Utf8String *str_ptr;
            };

            if (idx_sv.GetInteger(&index)) {
                if (Array *arrayptr = hv->GetPointer<Array>()) {
                    if (index >= arrayptr->GetSize()) {
                        m_state.ThrowException(thread, Exception::OutOfBoundsException());
                    } else if (index < 0) {
                        // wrap around (python style)
                        index = arrayptr->GetSize() + index;
                        if (index < 0 || index >= arrayptr->GetSize()) {
                            m_state.ThrowException(thread, Exception::OutOfBoundsException());
                        } else {
                            thread->m_regs[dst] = arrayptr->AtIndex(index);
                        }
                    } else {
                        thread->m_regs[dst] = arrayptr->AtIndex(index);
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
        } else {
            m_state.ThrowException(thread, Exception::NullReferenceException());
        }

        break;
    }
    case LOAD_NULL: {
        uint8_t reg; bs->Read(&reg);

        Value &sv = thread->m_regs[reg];
        sv.m_type = Value::HEAP_POINTER;
        sv.m_value.ptr = nullptr;

        break;
    }
    case LOAD_TRUE: {
        uint8_t reg; bs->Read(&reg);

        Value &sv = thread->m_regs[reg];
        sv.m_type = Value::BOOLEAN;
        sv.m_value.b = true;

        break;
    }
    case LOAD_FALSE: {
        uint8_t reg; bs->Read(&reg);

        Value &sv = thread->m_regs[reg];
        sv.m_type = Value::BOOLEAN;
        sv.m_value.b = false;

        break;
    }
    case MOV_OFFSET: {
        uint16_t offset; bs->Read(&offset);
        uint8_t reg; bs->Read(&reg);

        // copy value from register to stack value at (sp - offset)
        thread->m_stack[thread->m_stack.GetStackPointer() - offset] =
            thread->m_regs[reg];

        break;
    }
    case MOV_INDEX: {
        uint16_t idx; bs->Read(&idx);
        uint8_t reg; bs->Read(&reg);

        // copy value from register to stack value at index
        // NOTE: storing on main thread
        MAIN_THREAD->m_stack[idx] = thread->m_regs[reg];

        break;
    }
    case MOV_MEM: {
        uint8_t dst; bs->Read(&dst);
        uint8_t idx; bs->Read(&idx);
        uint8_t src; bs->Read(&src);

        Value &sv = thread->m_regs[dst];
        ASSERT_MSG(sv.m_type == Value::HEAP_POINTER, "destination must be a pointer");

        if (HeapValue *hv = sv.m_value.ptr) {
            if (Object *objptr = hv->GetPointer<Object>()) {
                const vm::TypeInfo *type_ptr = objptr->GetTypePtr();
                
                ASSERT(type_ptr != nullptr);
                ASSERT_MSG(idx < type_ptr->GetSize(), "member index out of bounds");
                
                objptr->GetMember(idx).value = thread->m_regs[src];
            } else {
                m_state.ThrowException(thread,
                    Exception(utf::Utf8String("not a standard object")));
            }
        } else {
            m_state.ThrowException(thread, Exception::NullReferenceException());
        }

        break;
    }
    case MOV_ARRAYIDX: {
        uint8_t dst; bs->Read(&dst);
        uint32_t idx; bs->Read(&idx);
        uint8_t src; bs->Read(&src);

        Value &sv = thread->m_regs[dst];
        ASSERT_MSG(sv.m_type == Value::HEAP_POINTER, "destination must be a pointer");

        if (HeapValue *hv = sv.m_value.ptr) {
            if (Array *arrayptr = hv->GetPointer<Array>()) {
                if (idx >= arrayptr->GetSize()) {
                    m_state.ThrowException(thread, Exception::OutOfBoundsException());
                } else {
                    arrayptr->AtIndex(idx) = thread->m_regs[src];
                }
            } else {
                m_state.ThrowException(thread,
                    Exception(utf::Utf8String("object is not an array")));
            }
        } else {
            m_state.ThrowException(thread, Exception::NullReferenceException());
        }

        break;
    }
    case MOV_REG: {
        uint8_t dst; bs->Read(&dst);
        uint8_t src; bs->Read(&src);
        thread->m_regs[dst] = thread->m_regs[src];
        break;
    }
    case HAS_MEM_HASH: {
        uint8_t dst; bs->Read(&dst);
        uint8_t src; bs->Read(&src);
        uint32_t hash; bs->Read(&hash);

        Value &sv = thread->m_regs[src];
        ASSERT_MSG(sv.m_type == Value::HEAP_POINTER, "destination must be a pointer");


        Value &res = thread->m_regs[dst];
        res.m_type = Value::BOOLEAN;

        if (sv.m_value.ptr != nullptr) {
            if (Object *obj_ptr = sv.m_value.ptr->GetPointer<Object>()) {
                if (Member *mem_ptr = obj_ptr->LookupMemberFromHash(hash)) {
                    res.m_value.b = true;
                    // leave the case statement
                    break;
                }
            }
        }

        // not found, set it to false
        res.m_value.b = false;

        break;
    }
    case PUSH: {
        uint8_t reg; bs->Read(&reg);

        // push a copy of the register value to the top of the stack
        thread->m_stack.Push(thread->m_regs[reg]);

        break;
    }
    case POP: {
        thread->m_stack.Pop();
        break;
    }
    case POP_N: {
        uint8_t n; bs->Read(&n);
        thread->m_stack.Pop(n);
        break;
    }
    case PUSH_ARRAY: {
        uint8_t dst; bs->Read(&dst);
        uint8_t src; bs->Read(&src);

        Value &sv = thread->m_regs[dst];
        ASSERT_MSG(sv.m_type == Value::HEAP_POINTER, "destination must be a pointer");

        HeapValue *hv = sv.m_value.ptr;
        if (HeapValue *hv = sv.m_value.ptr) {
            if (Array *arrayptr = hv->GetPointer<Array>()) {
                arrayptr->Push(thread->m_regs[src]);
            } else {
                m_state.ThrowException(thread,
                    Exception(utf::Utf8String("object is not an array")));
            }
        } else {
            m_state.ThrowException(thread,
                Exception::NullReferenceException());
        }

        break;
    }
    case ECHO: {
        uint8_t reg; bs->Read(&reg);
        Print(thread->m_regs[reg]);
        break;
    }
    case ECHO_NEWLINE: {
        utf::fputs(UTF8_CSTR("\n"), stdout);
        break;
    }
    case JMP: {
        uint8_t reg; bs->Read(&reg);

        const Value &addr = thread->m_regs[reg];
        ASSERT_MSG(addr.m_type == Value::ADDRESS, "register must hold an address");

        bs->Seek(addr.m_value.addr);

        break;
    }
    case JE: {
        uint8_t reg; bs->Read(&reg);

        if (thread->m_regs.m_flags == EQUAL) {
            const Value &addr = thread->m_regs[reg];
            ASSERT_MSG(addr.m_type == Value::ADDRESS, "register must hold an address");

            bs->Seek(addr.m_value.addr);
        }

        break;
    }
    case JNE: {
        uint8_t reg; bs->Read(&reg);

        if (thread->m_regs.m_flags != EQUAL) {
            const Value &addr = thread->m_regs[reg];
            ASSERT_MSG(addr.m_type == Value::ADDRESS, "register must hold an address");

            bs->Seek(addr.m_value.addr);
        }

        break;
    }
    case JG: {
        uint8_t reg; bs->Read(&reg);

        if (thread->m_regs.m_flags == GREATER) {
            const Value &addr = thread->m_regs[reg];
            ASSERT_MSG(addr.m_type == Value::ADDRESS, "register must hold an address");

            bs->Seek(addr.m_value.addr);
        }

        break;
    }
    case JGE: {
        uint8_t reg; bs->Read(&reg);

        if (thread->m_regs.m_flags == GREATER || thread->m_regs.m_flags == EQUAL) {
            const Value &addr = thread->m_regs[reg];
            ASSERT_MSG(addr.m_type == Value::ADDRESS, "register must hold an address");

            bs->Seek(addr.m_value.addr);
        }

        break;
    }
    case CALL: {
        uint8_t reg; bs->Read(&reg);
        uint8_t num_args; bs->Read(&num_args);

        Invoke(thread, bs, thread->m_regs[reg], num_args);

        break;
    }
    case RET: {
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
        
        break;
    }
    case BEGIN_TRY: {
        // register that holds address of catch block
        uint8_t reg; bs->Read(&reg);

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

        break;
    }
    case END_TRY: {
        // pop the try catch info from the stack
        ASSERT(thread->m_stack.Top().m_type == Value::TRY_CATCH_INFO);
        ASSERT(thread->m_exception_state.m_try_counter < 0);

        thread->m_stack.Pop();
        thread->m_exception_state.m_try_counter--;

        break;
    }
    case NEW: {
        uint8_t dst; bs->Read(&dst);
        uint8_t src; bs->Read(&src);

        // read value from register
        Value &type_sv = thread->m_regs[src];
        ASSERT(type_sv.m_type == Value::HEAP_POINTER);

        TypeInfo *type_ptr = type_sv.m_value.ptr->GetPointer<TypeInfo>();
        ASSERT(type_ptr != nullptr);

        // allocate heap object
        HeapValue *hv = m_state.HeapAlloc(thread);
        ASSERT(hv != nullptr);
        
        // create the Object from the info type_ptr provides us with.
        hv->Assign(Object(type_ptr, type_sv));

        // assign register value to the allocated object
        Value &sv = thread->m_regs[dst];
        sv.m_type = Value::HEAP_POINTER;
        sv.m_value.ptr = hv;

        break;
    }
    case NEW_ARRAY: {
        uint8_t dst; bs->Read(&dst);
        uint32_t size; bs->Read(&size);

        // allocate heap object
        HeapValue *hv = m_state.HeapAlloc(thread);
        ASSERT(hv != nullptr);

        hv->Assign(Array(size));

        // assign register value to the allocated object
        Value &sv = thread->m_regs[dst];
        sv.m_type = Value::HEAP_POINTER;
        sv.m_value.ptr = hv;

        break;
    }
    case CMP: {
        uint8_t lhs_reg; bs->Read(&lhs_reg);
        uint8_t rhs_reg; bs->Read(&rhs_reg);

        // dropout early for comparing something against itself
        if (lhs_reg == rhs_reg) {
            thread->m_regs.m_flags = EQUAL;
            break;
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
        } else if (lhs->m_type == Value::HEAP_POINTER && rhs->m_type == Value::HEAP_POINTER) {
            CompareAsPointers(thread, lhs, rhs);
        } else if (lhs->m_type == Value::FUNCTION && rhs->m_type == Value::FUNCTION) {
            CompareAsFunctions(thread, lhs, rhs);
        } else if (lhs->m_type == Value::NATIVE_FUNCTION && rhs->m_type == Value::NATIVE_FUNCTION) {
            CompareAsNativeFunctions(thread, lhs, rhs);
        } else {
            THROW_COMPARISON_ERROR(lhs->GetTypeString(), rhs->GetTypeString());
        }

        break;
    }
    case CMPZ: {
        uint8_t lhs_reg; bs->Read(&lhs_reg);

        // load values from registers
        Value *lhs = &thread->m_regs[lhs_reg];

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
            m_state.ThrowException(thread, Exception(utf::Utf8String(buffer)));
        }

        break;
    }
    case ADD: {
        uint8_t lhs_reg; bs->Read(&lhs_reg);
        uint8_t rhs_reg; bs->Read(&rhs_reg);
        uint8_t dst_reg; bs->Read(&dst_reg);

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
            m_state.ThrowException(thread, Exception(utf::Utf8String(buffer)));
        }

        // set the destination register to be the result
        thread->m_regs[dst_reg] = result;

        break;
    }
    case SUB: {
        uint8_t lhs_reg; bs->Read(&lhs_reg);
        uint8_t rhs_reg; bs->Read(&rhs_reg);
        uint8_t dst_reg; bs->Read(&dst_reg);

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
            m_state.ThrowException(thread, Exception(utf::Utf8String(buffer)));
        }

        // set the destination register to be the result
        thread->m_regs[dst_reg] = result;

        break;
    }
    case MUL: {
        uint8_t lhs_reg; bs->Read(&lhs_reg);
        uint8_t rhs_reg; bs->Read(&rhs_reg);
        uint8_t dst_reg; bs->Read(&dst_reg);

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
            m_state.ThrowException(thread, Exception(utf::Utf8String(buffer)));
        }

        // set the destination register to be the result
        thread->m_regs[dst_reg] = result;

        break;
    }
    case DIV: {
        uint8_t lhs_reg; bs->Read(&lhs_reg);
        uint8_t rhs_reg; bs->Read(&rhs_reg);
        uint8_t dst_reg; bs->Read(&dst_reg);

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
                m_state.ThrowException(thread, Exception::DivisionByZeroException());
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
                m_state.ThrowException(thread, Exception::DivisionByZeroException());
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
            m_state.ThrowException(thread, Exception(utf::Utf8String(buffer)));
        }

        // set the destination register to be the result
        thread->m_regs[dst_reg] = result;

        break;
    }
    case NEG: {
        uint8_t reg; bs->Read(&reg);

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
}

void VM::Execute(BytecodeStream *bs)
{
    ASSERT(bs != nullptr);
    ASSERT(m_state.GetNumThreads() > 0);

    uint8_t code;

    while (!bs->Eof() && m_state.good) {
        bs->Read(&code);
        HandleInstruction(MAIN_THREAD, bs, code);
    }
}

} // namespace vm
} // namespace ace
