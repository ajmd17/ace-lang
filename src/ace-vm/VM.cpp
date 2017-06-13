#include <ace-vm/VM.hpp>
#include <ace-vm/Value.hpp>
#include <ace-vm/HeapValue.hpp>
#include <ace-vm/Array.hpp>
#include <ace-vm/Object.hpp>
#include <ace-vm/TypeInfo.hpp>
#include <ace-vm/InstructionHandler.hpp>

#include <common/typedefs.hpp>
#include <common/instructions.hpp>
#include <common/utf8.hpp>
#include <common/hasher.hpp>
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
    m_state.MAIN_THREAD->m_stack.Push(sv);
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
            if (value.m_value.ptr == nullptr) {
                // special case for null pointers
                utf::fputs(UTF8_CSTR("null"), stdout);
            } else if (utf::Utf8String *str = value.m_value.ptr->GetPointer<utf::Utf8String>()) {
                // print string value
                utf::cout << *str;
            } else if (Array *array = value.m_value.ptr->GetPointer<Array>()) {
                // print array list
                const char sep_str[] = ", ";
                const size_t sep_str_len = std::strlen(sep_str);

                int buffer_index = 1;
                const int buffer_size = 256;
                utf::Utf8String res("[", buffer_size);

                // convert all array elements to string
                const size_t size = array->GetSize();
                for (size_t i = 0; i < size; i++) {
                    utf::Utf8String item_str = array->AtIndex(i).ToString();
                    size_t len = item_str.GetLength();

                    const bool last = i != size - 1;
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
        default: utf::cout << value.GetTypeString(); break;
    }
}

void VM::Invoke(InstructionHandler *handler,
    const Value &value,
    uint8_t nargs)
{
    VMState *state = handler->state;
    ExecutionThread *thread = handler->thread;
    BytecodeStream *bs = handler->bs;

    ASSERT(state != nullptr);
    ASSERT(thread != nullptr);
    ASSERT(bs != nullptr);

    if (value.m_type != Value::FUNCTION) {
        if (value.m_type == Value::NATIVE_FUNCTION) {
            Value **args = new Value*[nargs > 0 ? nargs : 1];

            int i = (int)thread->m_stack.GetStackPointer() - 1;
            for (int j = nargs - 1; j >= 0 && i >= 0; i--, j--) {
                args[j] = &thread->m_stack[i];
            }

            ace::sdk::Params params;
            params.handler = handler;
            params.args = args;
            params.nargs = nargs;

            // call the native function
            value.m_value.native_func(params);

            delete[] args;

            return;
        } else if (value.m_type == Value::HEAP_POINTER) {
            if (value.m_value.ptr == nullptr) {
                state->ThrowException(
                    thread,
                    Exception::NullReferenceException()
                );
                return;
            } else if (Object *object = value.m_value.ptr->GetPointer<Object>()) {
                if (Member *member = object->LookupMemberFromHash(hash_fnv_1("$invoke"))) {
                    const int sp = (int)thread->m_stack.GetStackPointer();
                    const int args_start = sp - nargs;

                    if (nargs > 0) {
                        // shift over by 1 -- and insert 'self' to start of args
                        // make a copy of last item to not overwrite it
                        thread->m_stack.Push(thread->m_stack[sp - 1]);

                        for (size_t i = args_start; i < sp - 1; i++) {
                            thread->m_stack[i + 1] = thread->m_stack[i];
                        }
                        
                        // set 'self' object to start of args
                        thread->m_stack[args_start] = value;
                    } else {
                        thread->m_stack.Push(value);
                    }

                    VM::Invoke(
                        handler,
                        member->value,
                        nargs + 1
                    );

                    Value &top = thread->m_stack.Top();
                    ASSERT(top.m_type == Value::FUNCTION_CALL);

                    // bookkeeping to remove the closure object
                    // normally, arguments are popped after the call is returned,
                    // rather than within the body
                    top.m_value.call.varargs_push--;

                    return;
                }
            }
        }

        char buffer[255];
        std::sprintf(
            buffer,
            "cannot invoke type '%s' as a function",
            value.GetTypeString()
        );

        state->ThrowException(
            thread,
            Exception(buffer)
        );

        return;
    }
    
    if ((value.m_value.func.m_flags & FunctionFlags::VARIADIC) && nargs < value.m_value.func.m_nargs - 1) {
        // if variadic, make sure the arg count is /at least/ what is required
        state->ThrowException(
            thread,
            Exception::InvalidArgsException(
                value.m_value.func.m_nargs,
                nargs,
                true
            )
        );
    } else if (!(value.m_value.func.m_flags & FunctionFlags::VARIADIC) && value.m_value.func.m_nargs != nargs) {
        state->ThrowException(
            thread,
            Exception::InvalidArgsException(
                value.m_value.func.m_nargs,
                nargs
            )
        );
    } else {
        Value previous_addr;
        previous_addr.m_type = Value::FUNCTION_CALL;
        previous_addr.m_value.call.varargs_push = 0;
        // store current address
        previous_addr.m_value.call.addr = (uint32_t)bs->Position();

        if (value.m_value.func.m_flags & FunctionFlags::VARIADIC) {
            // for each argument that is over the expected size, we must pop it from
            // the stack and add it to a new array.
            int varargs_amt = nargs - value.m_value.func.m_nargs + 1;
            if (varargs_amt < 0) {
                varargs_amt = 0;
            }
            
            // set varargs_push value so we know how to get back to the stack size before.
            previous_addr.m_value.call.varargs_push = varargs_amt - 1;
            
            // allocate heap object
            HeapValue *hv = state->HeapAlloc(thread);
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

void VM::HandleInstruction(InstructionHandler *handler, uint8_t code)
{
    std::lock_guard<std::mutex> lock(mtx);

    ExecutionThread *thread = handler->thread;
    BytecodeStream *bs = handler->bs;
    
    if (thread->m_exception_state.HasExceptionOccurred()) {
        if (thread->m_exception_state.m_try_counter > 0) {
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

        handler->StoreStaticString(
            len,
            str
        );

        delete[] str;

        break;
    }
    case STORE_STATIC_ADDRESS: {
        bc_address_t addr; bs->Read(&addr);

        handler->StoreStaticAddress(
            addr
        );

        break;
    }
    case STORE_STATIC_FUNCTION: {
        bc_address_t addr; bs->Read(&addr);
        uint8_t nargs; bs->Read(&nargs);
        uint8_t flags; bs->Read(&flags);

        handler->StoreStaticFunction(
            addr,
            nargs,
            flags
        );

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

        handler->StoreStaticType(
            type_name,
            size,
            names
        );

        delete[] type_name;
        
        // delete the names
        for (size_t i = 0; i < size; i++) {
            delete[] names[i];
        }

        delete[] names;

        break;
    }
    case LOAD_I32: {
        bc_reg_t reg; bs->Read(&reg);
        int32_t i32; bs->Read(&i32);

        handler->LoadI32(
            reg,
            i32
        );

        break;
    }
    case LOAD_I64: {
        bc_reg_t reg; bs->Read(&reg);
        int64_t i64; bs->Read(&i64);

        handler->LoadI64(
            reg,
            i64
        );

        break;
    }
    case LOAD_F32: {
        bc_reg_t reg; bs->Read(&reg);
        float f32; bs->Read(&f32);

        handler->LoadF32(
            reg,
            f32
        );

        break;
    }
    case LOAD_F64: {
        bc_reg_t reg; bs->Read(&reg);
        double f64; bs->Read(&f64);

        handler->LoadF64(
            reg,
            f64
        );

        break;
    }
    case LOAD_OFFSET: {
        bc_reg_t reg; bs->Read(&reg);
        uint16_t offset; bs->Read(&offset);

        handler->LoadOffset(
            reg,
            offset
        );

        break;
    }
    case LOAD_INDEX: {
        bc_reg_t reg; bs->Read(&reg);
        uint16_t index; bs->Read(&index);

        handler->LoadIndex(
            reg,
            index
        );

        break;
    }
    case LOAD_STATIC: {
        bc_reg_t reg; bs->Read(&reg);
        uint16_t index; bs->Read(&index);

        handler->LoadStatic(
            reg,
            index
        );

        break;
    }
    case LOAD_STRING: {
        bc_reg_t reg; bs->Read(&reg);
        // get string length
        uint32_t len; bs->Read(&len);

        // read string based on length
        char *str = new char[len + 1];
        str[len] = '\0';
        bs->Read(str, len);

        handler->LoadString(
            reg,
            len,
            str
        );

        delete[] str;

        break;
    }
    case LOAD_ADDR: {
        bc_reg_t reg; bs->Read(&reg);
        bc_address_t addr; bs->Read(&addr);

        handler->LoadAddr(
            reg,
            addr
        );

        break;
    }
    case LOAD_FUNC: {
        bc_reg_t reg; bs->Read(&reg);
        bc_address_t addr; bs->Read(&addr);
        uint8_t nargs; bs->Read(&nargs);
        uint8_t flags; bs->Read(&flags);

        handler->LoadFunc(
            reg,
            addr,
            nargs,
            flags
        );

        break;
    }
    case LOAD_TYPE: {
        bc_reg_t reg; bs->Read(&reg);
        uint16_t type_name_len; bs->Read(&type_name_len);

        char *type_name = new char[type_name_len + 1];
        type_name[type_name_len] = '\0';
        bs->Read(type_name, type_name_len);

        // number of members
        uint16_t size;
        bs->Read(&size);

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

        handler->LoadType(
            reg,
            type_name_len,
            type_name,
            size,
            names
        );

        delete[] type_name;
        
        // delete the names
        for (size_t i = 0; i < size; i++) {
            delete[] names[i];
        }

        delete[] names;

        break;
    }
    case LOAD_MEM: {
        bc_reg_t dst; bs->Read(&dst);
        bc_reg_t src; bs->Read(&src);
        uint8_t index; bs->Read(&index);

        handler->LoadMem(
            dst,
            src,
            index
        );

        break;
    }
    case LOAD_MEM_HASH: {
        bc_reg_t dst; bs->Read(&dst);
        bc_reg_t src; bs->Read(&src);
        uint32_t hash; bs->Read(&hash);

        handler->LoadMemHash(
            dst,
            src,
            hash
        );

        break;
    }
    case LOAD_ARRAYIDX: {
        bc_reg_t dst_reg; bs->Read(&dst_reg);
        bc_reg_t src_reg; bs->Read(&src_reg);
        bc_reg_t index_reg; bs->Read(&index_reg);

        handler->LoadArrayIdx(
            dst_reg,
            src_reg,
            index_reg
        );

        break;
    }
    case LOAD_NULL: {
        bc_reg_t reg; bs->Read(&reg);

        handler->LoadNull(
            reg
        );

        break;
    }
    case LOAD_TRUE: {
        bc_reg_t reg; bs->Read(&reg);

        handler->LoadTrue(
            reg
        );

        break;
    }
    case LOAD_FALSE: {
        bc_reg_t reg; bs->Read(&reg);

        handler->LoadFalse(
            reg
        );

        break;
    }
    case MOV_OFFSET: {
        uint16_t offset; bs->Read(&offset);
        bc_reg_t reg; bs->Read(&reg);

        handler->MovOffset(
            offset,
            reg
        );

        break;
    }
    case MOV_INDEX: {
        uint16_t index; bs->Read(&index);
        bc_reg_t reg; bs->Read(&reg);

        handler->MovIndex(
            index,
            reg
        );

        break;
    }
    case MOV_MEM: {
        bc_reg_t dst; bs->Read(&dst);
        uint8_t index; bs->Read(&index);
        bc_reg_t src; bs->Read(&src);

        handler->MovMem(
            dst,
            index,
            src
        );

        break;
    }
    case MOV_MEM_HASH: {
        bc_reg_t dst; bs->Read(&dst);
        uint32_t hash; bs->Read(&hash);
        bc_reg_t src; bs->Read(&src);

        handler->MovMemHash(
            dst,
            hash,
            src
        );

        break;
    }
    case MOV_ARRAYIDX: {
        bc_reg_t dst; bs->Read(&dst);
        uint32_t index; bs->Read(&index);
        bc_reg_t src; bs->Read(&src);

        handler->MovArrayIdx(
            dst,
            index,
            src
        );

        break;
    }
    case MOV_REG: {
        bc_reg_t dst; bs->Read(&dst);
        bc_reg_t src; bs->Read(&src);
        
        handler->MovReg(
            dst,
            src
        );

        break;
    }
    case HAS_MEM_HASH: {
        bc_reg_t dst; bs->Read(&dst);
        bc_reg_t src; bs->Read(&src);
        uint32_t hash; bs->Read(&hash);

        handler->HasMemHash(
            dst,
            src,
            hash
        );

        break;
    }
    case PUSH: {
        bc_reg_t reg; bs->Read(&reg);

        handler->Push(
            reg
        );

        break;
    }
    case POP: {
        handler->Pop();

        break;
    }
    case POP_N: {
        uint8_t n; bs->Read(&n);
        
        handler->PopN(
            n
        );

        break;
    }
    case PUSH_ARRAY: {
        bc_reg_t dst; bs->Read(&dst);
        bc_reg_t src; bs->Read(&src);

        handler->PushArray(
            dst,
            src
        );

        break;
    }
    case ECHO: {
        bc_reg_t reg; bs->Read(&reg);
        handler->Echo(
            reg
        );

        break;
    }
    case ECHO_NEWLINE: {
        handler->EchoNewline();

        break;
    }
    case JMP: {
        bc_reg_t reg; bs->Read(&reg);

        handler->Jmp(
            reg
        );

        break;
    }
    case JE: {
        bc_reg_t reg; bs->Read(&reg);

        handler->Je(
            reg
        );

        break;
    }
    case JNE: {
        bc_reg_t reg; bs->Read(&reg);

        handler->Jne(
            reg
        );

        break;
    }
    case JG: {
        bc_reg_t reg; bs->Read(&reg);

        handler->Jg(
            reg
        );

        break;
    }
    case JGE: {
        bc_reg_t reg; bs->Read(&reg);

        handler->Jge(
            reg
        );

        break;
    }
    case CALL: {
        bc_reg_t reg; bs->Read(&reg);
        uint8_t nargs; bs->Read(&nargs);

        handler->Call(
            reg,
            nargs
        );

        break;
    }
    case RET: {
        handler->Ret();
        
        break;
    }
    case BEGIN_TRY: {
        // register that holds address of catch block
        bc_reg_t reg; bs->Read(&reg);

        handler->BeginTry(
            reg
        );

        break;
    }
    case END_TRY: {
        handler->EndTry();

        break;
    }
    case NEW: {
        bc_reg_t dst; bs->Read(&dst);
        bc_reg_t src; bs->Read(&src);

        handler->New(
            dst,
            src
        );

        break;
    }
    case NEW_ARRAY: {
        bc_reg_t dst; bs->Read(&dst);
        uint32_t size; bs->Read(&size);

        handler->NewArray(
            dst,
            size
        );

        break;
    }
    case CMP: {
        bc_reg_t lhs_reg; bs->Read(&lhs_reg);
        bc_reg_t rhs_reg; bs->Read(&rhs_reg);

        handler->Cmp(
            lhs_reg,
            rhs_reg
        );

        break;
    }
    case CMPZ: {
        bc_reg_t reg; bs->Read(&reg);

        handler->CmpZ(
            reg
        );

        break;
    }
    case ADD: {
        bc_reg_t lhs_reg; bs->Read(&lhs_reg);
        bc_reg_t rhs_reg; bs->Read(&rhs_reg);
        bc_reg_t dst_reg; bs->Read(&dst_reg);

        handler->Add(
            lhs_reg,
            rhs_reg,
            dst_reg
        );

        break;
    }
    case SUB: {
        bc_reg_t lhs_reg; bs->Read(&lhs_reg);
        bc_reg_t rhs_reg; bs->Read(&rhs_reg);
        bc_reg_t dst_reg; bs->Read(&dst_reg);

        handler->Sub(
            lhs_reg,
            rhs_reg,
            dst_reg
        );

        break;
    }
    case MUL: {
        bc_reg_t lhs_reg; bs->Read(&lhs_reg);
        bc_reg_t rhs_reg; bs->Read(&rhs_reg);
        bc_reg_t dst_reg; bs->Read(&dst_reg);

        handler->Mul(
            lhs_reg,
            rhs_reg,
            dst_reg
        );

        break;
    }
    case DIV: {
        bc_reg_t lhs_reg; bs->Read(&lhs_reg);
        bc_reg_t rhs_reg; bs->Read(&rhs_reg);
        bc_reg_t dst_reg; bs->Read(&dst_reg);

        handler->Div(
            lhs_reg,
            rhs_reg,
            dst_reg
        );

        break;
    }
    case MOD: {
        bc_reg_t lhs_reg; bs->Read(&lhs_reg);
        bc_reg_t rhs_reg; bs->Read(&rhs_reg);
        bc_reg_t dst_reg; bs->Read(&dst_reg);

        handler->Mod(
            lhs_reg,
            rhs_reg,
            dst_reg
        );

        break;
    }
    case AND: {
        bc_reg_t lhs_reg; bs->Read(&lhs_reg);
        bc_reg_t rhs_reg; bs->Read(&rhs_reg);
        bc_reg_t dst_reg; bs->Read(&dst_reg);

        handler->And(
            lhs_reg,
            rhs_reg,
            dst_reg
        );

        break;
    }
    case OR: {
        bc_reg_t lhs_reg; bs->Read(&lhs_reg);
        bc_reg_t rhs_reg; bs->Read(&rhs_reg);
        bc_reg_t dst_reg; bs->Read(&dst_reg);

        handler->Or(
            lhs_reg,
            rhs_reg,
            dst_reg
        );

        break;
    }
    case XOR: {
        bc_reg_t lhs_reg; bs->Read(&lhs_reg);
        bc_reg_t rhs_reg; bs->Read(&rhs_reg);
        bc_reg_t dst_reg; bs->Read(&dst_reg);

        handler->Xor(
            lhs_reg,
            rhs_reg,
            dst_reg
        );

        break;
    }
    case SHL: {
        bc_reg_t lhs_reg; bs->Read(&lhs_reg);
        bc_reg_t rhs_reg; bs->Read(&rhs_reg);
        bc_reg_t dst_reg; bs->Read(&dst_reg);

        handler->Shl(
            lhs_reg,
            rhs_reg,
            dst_reg
        );

        break;
    }
    case SHR: {
        bc_reg_t lhs_reg; bs->Read(&lhs_reg);
        bc_reg_t rhs_reg; bs->Read(&rhs_reg);
        bc_reg_t dst_reg; bs->Read(&dst_reg);

        handler->Shr(
            lhs_reg,
            rhs_reg,
            dst_reg
        );

        break;
    }
    case NEG: {
        bc_reg_t reg; bs->Read(&reg);

        handler->Neg(
            reg
        );

        break;
    }
    default: {
        int64_t last_pos = (int64_t)bs->Position() - sizeof(uint8_t);
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

    InstructionHandler handler(
        &m_state,
        m_state.MAIN_THREAD,
        bs
    );

    uint8_t code;

    while (!bs->Eof() && m_state.good) {
        bs->Read(&code);
        HandleInstruction(&handler, code);
    }
}

} // namespace vm
} // namespace ace
