/*
.... $MM ....... . . .... . . ... ...... ....... ..MM?  ....
....MM, ........................................... :MM.....
....MM...............................................MM.....
....MM........+Z?............:$$=.........IZI........MM.....
....MM.....?ZO...+Z,Z.....~ZZ....ZZ+...ZZI...ZZZ.....MM.....
...7M:....IZ.... ..$Z....?Z.... ......OZ.. . ..O$. ..$M=. .
. MM......Z:........O....Z,.... . . ..ZZZZZZZZZZZ. ... MM..
...NM.....Z.........Z....Z... ........Z..............:MO....
....MM....OZ.......+Z....ZZ...........ZZ.......$~....MM.....
....MM.....$Z+....Z$Z.....ZZ7....$ZZ...ZZ....:ZZ.....MM.....
....MM.......?ZZZI. ?.......+ZZZZ:.......7ZZZI.......MM.....
....MM...............................................MM.....
.....MMM .................... .....................MMM .....
*/

#include <ace/API.hpp>
#include <ace/Runtime.hpp>

#include <ace-c/ace-c.hpp>
#include <ace-c/Configuration.hpp>
#include <ace-c/SemanticAnalyzer.hpp>
#include <ace-c/Optimizer.hpp>
#include <ace-c/Lexer.hpp>
#include <ace-c/Parser.hpp>
#include <ace-c/Compiler.hpp>
#include <ace-c/dis/DecompilationUnit.hpp>
#include <ace-c/emit/cppgen/CppGenerator.hpp>

#include <ace-vm/Object.hpp>
#include <ace-vm/Array.hpp>
#include <ace-vm/Value.hpp>
#include <ace-vm/InstructionHandler.hpp>

#include <common/cli_args.hpp>
#include <common/str_util.hpp>
#include <common/instructions.hpp>
#include <common/typedefs.hpp>
#include <common/timer.hpp>
#include <common/termcolor.hpp>
#include <common/hasher.hpp>

#include <vector>
#include <string>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <mutex>
#include <random>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <ctime>

using namespace ace;

std::mutex mtx;
// all cpp threads
std::vector<std::thread> threads;

std::string exec_path;

void Events_call_action(ace::sdk::Params params)
{
    ACE_CHECK_ARGS(>=, 2);

    vm::Value *target_ptr = params.args[0];
    ASSERT(target_ptr != nullptr);

    vm::Value *value_ptr = params.args[1];
    ASSERT(value_ptr != nullptr);

    vm::Exception ex("call_action() expects an Object or Function as the first argument");
    vm::Exception ex1("Each item in event array should be of type Array");

    if (value_ptr->m_type == vm::Value::FUNCTION &&
       (value_ptr->m_value.func.m_flags & FunctionFlags::GENERATOR)) {
        // if the key is a generator, it should look like this internally:
        /*
            () {
                return (<arguments>, callback) {
                    // ...
                    callback(...)
                }
            }
        */
        // so we should invoke the object which will return the nested closure,
        // and pass our found handler to it (at the end of this function)
        // as `callback`
        std::swap(*target_ptr, *value_ptr);
    } else if (value_ptr->m_type == vm::Value::HEAP_POINTER &&
               value_ptr->m_value.ptr != nullptr) {
        if (vm::Object *object = value_ptr->m_value.ptr->GetPointer<vm::Object>()) {
            if (vm::Member *member = object->LookupMemberFromHash(hash_fnv_1("$invoke"))) {
                if (member->value.m_type == vm::Value::FUNCTION &&
                   (member->value.m_value.func.m_flags & FunctionFlags::GENERATOR)) {
                    // value is a generator, so swap.
                    std::swap(*target_ptr, *value_ptr);
                }
            }
        }
    }

    switch (target_ptr->GetType()) {
        case vm::Value::HEAP_POINTER: {
            if (target_ptr->m_value.ptr == nullptr) {
                goto return_null_handler;
            }

            // lookup '$events' member
            if (vm::Object *object = target_ptr->m_value.ptr->GetPointer<vm::Object>()) {

                if (vm::Member *member = object->LookupMemberFromHash(hash_fnv_1("$events"))) {
                    // $events member found
                    if (member->value.m_type != vm::Value::ValueType::HEAP_POINTER) {
                        params.handler->state->ThrowException(
                            params.handler->thread,
                            vm::Exception("$events must be an Array")
                        );
                        return;
                    }

                    if (vm::Array *array = member->value.m_value.ptr->GetPointer<vm::Array>()) {
                        // used for comparing values
                        union {
                            aint64 i;
                            afloat64 f;
                        } a, b;

                        enum {
                            MATCH_TYPES,
                            MATCH_VALUES
                        } match_mode = MATCH_VALUES;

                        vm::Value *res_func = nullptr;

                        // look thru all array items until we find a match.
                        for (size_t i = 0; i < array->GetSize(); i++) {
                            vm::Value &el = array->AtIndex(i);

                            if (el.m_type != vm::Value::HEAP_POINTER) {
                                params.handler->state->ThrowException(
                                    params.handler->thread,
                                    ex1
                                );
                                return;
                            }

                            // make sure each item is an array (this could be changed to a tuple,
                            // or something more efficient?)
                            if (vm::Array *el_array = el.m_value.ptr->GetPointer<vm::Array>()) {
                                if (el_array->GetSize() < 2) { // each item has at least 2 elements
                                    params.handler->state->ThrowException(
                                        params.handler->thread,
                                        vm::Exception::OutOfBoundsException()
                                    );
                                    return;
                                }

                                // el[0]: the value to test against
                                // el[1]: the handler function
                                vm::Value &handler_value = el_array->AtIndex(0);
                                vm::Value &handler_func = el_array->AtIndex(1);

                                // compare integers
                                if (value_ptr->GetInteger(&a.i) && handler_value.GetInteger(&b.i)) {
                                    if (match_mode == MATCH_TYPES || a.i == b.i) {
                                        res_func = &handler_func;
                                        break;
                                    }
                                } else if (value_ptr->GetNumber(&a.f) && handler_value.GetNumber(&b.f)) {
                                    if (match_mode == MATCH_TYPES || a.f == b.f) {
                                        res_func = &handler_func;
                                        break;
                                    }
                                } else if (value_ptr->m_type == vm::Value::BOOLEAN && handler_value.m_type == vm::Value::BOOLEAN) {
                                    if (match_mode == MATCH_TYPES || value_ptr->m_value.b == handler_value.m_value.b) {
                                        res_func = &handler_func;
                                        break;
                                    }
                                } else if (value_ptr->m_type == vm::Value::HEAP_POINTER && handler_value.m_type == vm::Value::HEAP_POINTER) {
                                    vm::HeapValue *hv_a = value_ptr->m_value.ptr;
                                    vm::HeapValue *hv_b = handler_value.m_value.ptr;

                                    // drop out early for same pointer value
                                    if (hv_a == hv_b) {
                                        res_func = &handler_func;
                                        break;
                                    } else if (hv_a == nullptr || hv_b == nullptr) {
                                        // one is null... not same
                                        // continue.
                                    } else if (hv_a->GetTypeId() == hv_b->GetTypeId()) {
                                        if (match_mode == MATCH_TYPES || *hv_a == *hv_b) {
                                            res_func = &handler_func;
                                            break;
                                        }
                                    }
                                }
                            } else {
                                params.handler->state->ThrowException(
                                    params.handler->thread,
                                    ex1
                                );
                                return;
                            }
                        }

                        // handler found, call it
                        if (res_func != nullptr) {
                            // set value to be the 'self' object because it is a member function
                            *value_ptr = *target_ptr;
                            vm::VM::Invoke(
                                params.handler,
                                *res_func,
                                params.nargs - 1
                            );
                            return;
                        }
                    }
                } else if (vm::Member *member = object->LookupMemberFromHash(hash_fnv_1("$invoke"))) {
                    if (member->value.m_type == vm::Value::FUNCTION || 
                        member->value.m_type == vm::Value::NATIVE_FUNCTION) {
                        // callable object
                        vm::VM::Invoke(
                            params.handler,
                            member->value,
                            params.nargs
                        );
                        return;
                    }
                }
            }
        
        return_null_handler:
            // not found, return null
            vm::Value res;
            res.m_type = vm::Value::HEAP_POINTER;
            res.m_value.ptr = nullptr;
            ACE_RETURN(res);
        }
        case vm::Value::FUNCTION:
            // fallthrough
        case vm::Value::NATIVE_FUNCTION:
            // for functions, return the original function itself
            //ACE_RETURN(*target_ptr);
            vm::VM::Invoke(
                params.handler,
                *target_ptr,
                params.nargs - 1
            );
            return;
        default:
            params.handler->state->ThrowException(
                params.handler->thread,
                ex
            );
    }
}

void Events_get_action_handler(ace::sdk::Params params)
{
    ACE_CHECK_ARGS(==, 2);

    const vm::Value *target_ptr = params.args[0];
    ASSERT(target_ptr != nullptr);

    const vm::Value *value_ptr = params.args[1];
    ASSERT(value_ptr != nullptr);

    const vm::Value &value = *value_ptr;

    vm::Exception ex("get_action_handler() expects an Object or Function as the first argument");
    vm::Exception ex1("Each item in event array should be of type Array");

    switch (target_ptr->GetType()) {
        case vm::Value::ValueType::HEAP_POINTER: {
            if (target_ptr->m_value.ptr == nullptr) {
                goto return_null_handler;
            }

            // lookup '$events' member
            if (vm::Object *object = target_ptr->m_value.ptr->GetPointer<vm::Object>()) {
                const std::uint32_t hash = hash_fnv_1("$events");

                if (vm::Member *member = object->LookupMemberFromHash(hash)) {
                    // $events member found
                    if (member->value.m_type != vm::Value::ValueType::HEAP_POINTER) {
                        params.handler->state->ThrowException(
                            params.handler->thread,
                            vm::Exception("$events must be an Array")
                        );
                        return;
                    }

                    if (vm::Array *array = member->value.m_value.ptr->GetPointer<vm::Array>()) {
                        // used for comparing values
                        union {
                            aint64 i;
                            afloat64 f;
                        } a, b;

                        enum {
                            MATCH_TYPES,
                            MATCH_VALUES
                        } match_mode = MATCH_VALUES;

                        vm::Value *res_func = nullptr;

                        // look thru all array items until we find a match.
                        for (size_t i = 0; i < array->GetSize(); i++) {
                            vm::Value &el = array->AtIndex(i);

                            if (el.m_type != vm::Value::HEAP_POINTER) {
                                params.handler->state->ThrowException(
                                    params.handler->thread,
                                    ex1
                                );
                                return;
                            }

                            // make sure each item is an array (this could be changed to a tuple,
                            // or something more efficient?)
                            if (vm::Array *el_array = el.m_value.ptr->GetPointer<vm::Array>()) {
                                if (el_array->GetSize() < 2) { // each item has at least 2 elements
                                    params.handler->state->ThrowException(
                                        params.handler->thread,
                                        vm::Exception::OutOfBoundsException()
                                    );
                                    return;
                                }

                                // el[0]: the value to test against
                                // el[1]: the handler function
                                vm::Value &handler_value = el_array->AtIndex(0);
                                vm::Value &handler_func = el_array->AtIndex(1);

                                // compare integers
                                if (value.GetInteger(&a.i) && handler_value.GetInteger(&b.i)) {
                                    if (match_mode == MATCH_TYPES || a.i == b.i) {
                                        res_func = &handler_func;
                                        break;
                                    }
                                } else if (value.GetNumber(&a.f) && handler_value.GetNumber(&b.f)) {
                                    if (match_mode == MATCH_TYPES || a.f == b.f) {
                                        res_func = &handler_func;
                                        break;
                                    }
                                } else if (value.m_type == vm::Value::BOOLEAN && handler_value.m_type == vm::Value::BOOLEAN) {
                                    if (match_mode == MATCH_TYPES || value.m_value.b == handler_value.m_value.b) {
                                        res_func = &handler_func;
                                        break;
                                    }
                                } else if (value.m_type == vm::Value::HEAP_POINTER && handler_value.m_type == vm::Value::HEAP_POINTER) {
                                    vm::HeapValue *hv_a = value.m_value.ptr;
                                    vm::HeapValue *hv_b = handler_value.m_value.ptr;

                                    // drop out early for same pointer value
                                    if (hv_a == hv_b) {
                                        res_func = &handler_func;
                                        break;
                                    } else if (hv_a == nullptr || hv_b == nullptr) {
                                        // one is null... not same
                                        // continue.
                                    } else if (hv_a->GetTypeId() == hv_b->GetTypeId()) {
                                        if (match_mode == MATCH_TYPES || *hv_a == *hv_b) {
                                            res_func = &handler_func;
                                            break;
                                        }
                                    }
                                }
                            } else {
                                params.handler->state->ThrowException(
                                    params.handler->thread,
                                    ex1
                                );
                                return;
                            }
                        }

                        // handler found, return it
                        if (res_func != nullptr) {
                            ACE_RETURN(*res_func);
                        }
                    }
                }
            }
        
        return_null_handler:
            // not found, return null
            vm::Value res;
            res.m_type = vm::Value::HEAP_POINTER;
            res.m_value.ptr = nullptr;
            ACE_RETURN(res);
        }
        case vm::Value::ValueType::FUNCTION:
            // fallthrough
        case vm::Value::ValueType::NATIVE_FUNCTION:
            // for functions, return the original function itself
            ACE_RETURN(*target_ptr);
        default:
            params.handler->state->ThrowException(
                params.handler->thread,
                ex
            );
    }
}

void Time_now(ace::sdk::Params params)
{
    ACE_CHECK_ARGS(==, 0);

    std::int64_t unix_timestamp = std::chrono::seconds(std::time(NULL)).count();

    // return the timestamp
    vm::Value res;
    res.m_type = vm::Value::ValueType::I64;
    res.m_value.i64 = unix_timestamp;
    ACE_RETURN(res);
}

void Runtime_gc(ace::sdk::Params params)
{
    ACE_CHECK_ARGS(==, 0);

    const size_t heap_size_before = params.handler->state->GetHeap().Size();

    // run the gc
    params.handler->state->GC();

    const size_t heap_size_after = params.handler->state->GetHeap().Size();
    
    utf::cout << (heap_size_before - heap_size_after) << " object(s) collected.\n";
}

void Runtime_dump_heap(ace::sdk::Params params)
{
    ACE_CHECK_ARGS(==, 0);

    utf::cout << params.handler->state->GetHeap() << "\n";
}

void Runtime_dump_stack(ace::sdk::Params params)
{
    ACE_CHECK_ARGS(==, 0);

    utf::cout << params.handler->thread->GetStack() << "\n";
}

void Runtime_typeof(ace::sdk::Params params)
{
    ACE_CHECK_ARGS(==, 1);

    // create heap value for string
    vm::HeapValue *ptr = params.handler->state->HeapAlloc(params.handler->thread);
    ASSERT(ptr != nullptr);
    ptr->Assign(utf::Utf8String(params.args[0]->GetTypeString()));

    vm::Value res;
    // assign register value to the allocated object
    res.m_type = vm::Value::HEAP_POINTER;
    res.m_value.ptr = ptr;

    ACE_RETURN(res);
}

void Runtime_load_library(ace::sdk::Params params)
{
    ACE_CHECK_ARGS(==, 1);

    vm::Value *target_ptr = params.args[0];
    ASSERT(target_ptr != nullptr);

    vm::Exception e = vm::Exception(utf::Utf8String("load_library() expects a String as the first argument"));

    if (target_ptr->GetType() == vm::Value::ValueType::HEAP_POINTER) {
        if (target_ptr->GetValue().ptr == nullptr) {
            params.handler->state->ThrowException(
                params.handler->thread,
                vm::Exception::NullReferenceException()
            );
        } else if (utf::Utf8String *strptr = target_ptr->GetValue().ptr->GetPointer<utf::Utf8String>()) {
            // load library from string
            utf::Utf8String full_path;

            if (strptr->GetLength() > 0 && strptr->GetData()[0] == '/') {
                full_path = *strptr;
            } else {
                full_path = utf::Utf8String(exec_path.c_str()) + *strptr;
            }

            full_path += "." ACE_DYLIB_EXT;
            
            Library lib = Runtime::Load(full_path.GetData());

            if (lib.GetHandle() == nullptr) {
                // could not load library
                params.handler->state->ThrowException(
                    params.handler->thread,
                    vm::Exception::LibraryLoadException(full_path.GetData())
                );
            } else {
                // store the library in a variable

                // create heap value for the library
                vm::HeapValue *ptr = params.handler->state->HeapAlloc(params.handler->thread);
                ASSERT(ptr != nullptr);
                // assign it to the library
                ptr->Assign(lib);

                vm::Value res;
                // assign register value to the allocated object
                res.m_type = vm::Value::HEAP_POINTER;
                res.m_value.ptr = ptr;

                ACE_RETURN(res);
            }
        } else {
            params.handler->state->ThrowException(params.handler->thread, e);
        }
    } else {
        params.handler->state->ThrowException(params.handler->thread, e);
    }
}

void Runtime_load_function(ace::sdk::Params params)
{
    ACE_CHECK_ARGS(==, 2);

    vm::Value *arg0 = params.args[0];
    ASSERT(arg0 != nullptr);

    vm::Value *arg1 = params.args[1];
    ASSERT(arg1 != nullptr);

    vm::Exception e = vm::Exception(utf::Utf8String("load_function() expects arguments of type Library and String"));

    Library *lib_ptr = nullptr;
    utf::Utf8String *str_ptr = nullptr;

    if (arg0->GetType() == vm::Value::ValueType::HEAP_POINTER) {
        if (arg0->GetValue().ptr == nullptr) {
            params.handler->state->ThrowException(params.handler->thread, vm::Exception::NullReferenceException());
        } else if ((lib_ptr = arg0->GetValue().ptr->GetPointer<Library>()) == nullptr) {
            params.handler->state->ThrowException(params.handler->thread, e);
        } else {
            if (arg1->GetType() == vm::Value::ValueType::HEAP_POINTER) {
                if (arg1->GetValue().ptr == nullptr) {
                    params.handler->state->ThrowException(params.handler->thread, vm::Exception::NullReferenceException());
                } else if ((str_ptr = arg1->GetValue().ptr->GetPointer<utf::Utf8String>()) == nullptr) {
                    params.handler->state->ThrowException(params.handler->thread, e);
                } else {
                    auto func = lib_ptr->GetFunction(str_ptr->GetData());
                    if (!func) {
                        // could not load function from the library
                        params.handler->state->ThrowException(
                            params.handler->thread,
                            vm::Exception::LibraryFunctionLoadException(str_ptr->GetData())
                        );
                    } else {
                        vm::Value res;
                        res.m_type = vm::Value::NATIVE_FUNCTION;
                        res.m_value.native_func = func;

                        ACE_RETURN(res);
                    }
                }
            } else {
                params.handler->state->ThrowException(params.handler->thread, e);
            }
        }
    } else {
        params.handler->state->ThrowException(params.handler->thread, e);
    }
}

void Global_to_json(ace::sdk::Params params)
{
    ACE_CHECK_ARGS(==, 1);

    // get value
    vm::Value *target_ptr = params.args[0];
    ASSERT(target_ptr != nullptr);

    // convert to json string
    utf::Utf8String json_string(256);
    target_ptr->ToRepresentation(json_string, false /* do not add type names */);

    // store in memory
    vm::HeapValue *ptr = params.handler->state->HeapAlloc(params.handler->thread);
    ASSERT(ptr != nullptr);
    ptr->Assign(json_string);

    vm::Value res;
    // assign register value to the allocated object
    res.m_type = vm::Value::HEAP_POINTER;
    res.m_value.ptr = ptr;

    ACE_RETURN(res);
}

void Global_decompile(ace::sdk::Params params)
{
    ACE_CHECK_ARGS(==, 1);

    // get value
    vm::Value *target_ptr = params.args[0];
    ASSERT(target_ptr != nullptr);

    utf::Utf8String bytecode_str;

    if (target_ptr->m_type != vm::Value::FUNCTION) {
        if (target_ptr->m_type == vm::Value::NATIVE_FUNCTION) {
            bytecode_str += "<Native Code>";
        } else {
            char buffer[256];
            std::sprintf(
                buffer,
                "cannot convert type '%s' to bytecode",
                target_ptr->GetTypeString()
            );
            params.handler->state->ThrowException(
                params.handler->thread,
                vm::Exception(buffer)
            );
        }
    } else {
        ASSERT(params.handler->bs != nullptr);

        const size_t pos = target_ptr->m_value.func.m_addr;
        ASSERT(pos < params.handler->bs->Size());

        // create required objects
        // TODO: refactor this so it is shared across the
        // whole program, instead of having to copy the buffer
        SourceFile source_file(
            "",
            params.handler->bs->Size()
        );
        // read into it starting at pos
        source_file.ReadIntoBuffer(
            &params.handler->bs->GetBuffer()[pos],
            params.handler->bs->Size() - pos
        );

        // note: this is different than BytecodeStream
        // soon, it should be refactored into one
        vm::BytecodeStream byte_stream = vm::BytecodeStream::FromSourceFile(&source_file);

        // create DecompilationUnit
        DecompilationUnit dec;
        InstructionStream is;
        std::stringstream ss;

        uint8_t code;
        do {
            byte_stream.Read(&code);

            // decompile the instruction
            dec.DecodeNext(code, byte_stream, is, &ss);
        } while (code != RET && !byte_stream.Eof());

        std::string ss_str = ss.str();
        bytecode_str += ss_str.data();
    }
    
    // create heap value for string
    vm::HeapValue *ptr = params.handler->state->HeapAlloc(params.handler->thread);
    ASSERT(ptr != nullptr);
    ptr->Assign(bytecode_str);

    vm::Value res;
    // assign register value to the allocated object
    res.m_type = vm::Value::HEAP_POINTER;
    res.m_value.ptr = ptr;

    ACE_RETURN(res);
}

void Global_prompt(ace::sdk::Params params)
{
    ACE_CHECK_ARGS(==, 1);

    vm::Value *target_ptr = params.args[0];
    ASSERT(target_ptr != nullptr);

    vm::Exception e = vm::Exception(utf::Utf8String("prompt() expects a String as the first argument"));

    if (target_ptr->GetType() == vm::Value::ValueType::HEAP_POINTER) {
        if (target_ptr->GetValue().ptr == nullptr) {
            params.handler->state->ThrowException(params.handler->thread, vm::Exception::NullReferenceException());
        } else if (utf::Utf8String *str_ptr = target_ptr->GetValue().ptr->GetPointer<utf::Utf8String>()) {
            utf::cout << (*str_ptr) << ' ';

            // read input...
            std::string line;
            if (std::getline(std::cin, line)) {
                // store the result in a variable
                vm::HeapValue *ptr = params.handler->state->HeapAlloc(params.handler->thread);
                ASSERT(ptr != nullptr);
                // assign it to the formatted string
                ptr->Assign(utf::Utf8String(line.data()));

                vm::Value res;
                // assign register value to the allocated object
                res.m_type = vm::Value::HEAP_POINTER;
                res.m_value.ptr = ptr;

                ACE_RETURN(res);
            } else {
                // error, throw exception
                params.handler->state->ThrowException(
                    params.handler->thread,
                    vm::Exception("Error reading input from stdin")
                );
            }
        } else {
            params.handler->state->ThrowException(params.handler->thread, e);
        }
    } else {
        params.handler->state->ThrowException(params.handler->thread, e);
    }
}

void Global_to_string(ace::sdk::Params params)
{
    ACE_CHECK_ARGS(==, 1);

    // create heap value for string
    vm::HeapValue *ptr = params.handler->state->HeapAlloc(params.handler->thread);
    ASSERT(ptr != nullptr);
    ptr->Assign(params.args[0]->ToString());

    vm::Value res;
    // assign register value to the allocated object
    res.m_type = vm::Value::HEAP_POINTER;
    res.m_value.ptr = ptr;

    ACE_RETURN(res);
}

void Global_fmt(ace::sdk::Params params)
{
    ACE_CHECK_ARGS(>=, 1);

    vm::Value *target_ptr = params.args[0];
    ASSERT(target_ptr != nullptr);

    vm::Exception e = vm::Exception(utf::Utf8String("fmt() expects a String as the first argument"));

    if (target_ptr->GetType() == vm::Value::ValueType::HEAP_POINTER) {
        if (target_ptr->GetValue().ptr == nullptr) {
            params.handler->state->ThrowException(params.handler->thread, vm::Exception::NullReferenceException());
        } else if (utf::Utf8String *str_ptr = target_ptr->GetValue().ptr->GetPointer<utf::Utf8String>()) {
            // scan through string and merge each argument where there is a '%'
            const size_t original_length = str_ptr->GetLength();
            utf::Utf8String result_string(original_length);

            const char *original_data = str_ptr->GetData();
            ASSERT(original_data != nullptr);

            const int buffer_size = 256;
            char buffer[buffer_size + 1] = {'\0'};

            // number of '%' characters handled
            int num_fmts = 0;
            int buffer_idx = 0;
            
            for (size_t i = 0; i < original_length; i++) {

                if (original_data[i] == '%' && num_fmts < params.nargs - 1) {
                    // set end of buffer to be NUL
                    buffer[buffer_idx + 1] = '\0';
                    // now upload to result string
                    result_string += buffer;
                    // clear buffer
                    buffer_idx = 0;
                    buffer[0] = '\0';

                    result_string += params.args[++num_fmts]->ToString();
                } else {
                    buffer[buffer_idx] = original_data[i];

                    if (buffer_idx == buffer_size - 1 || i == original_length - 1) {
                        // set end of buffer to be NUL
                        buffer[buffer_idx + 1] = '\0';
                        // now upload to result string
                        result_string += buffer;
                        //clear buffer
                        buffer_idx = 0;
                        buffer[0] = '\0';
                    } else {
                        buffer_idx++;
                    }
                }
            }

            // store the result in a variable
            vm::HeapValue *ptr = params.handler->state->HeapAlloc(params.handler->thread);
            ASSERT(ptr != nullptr);
            // assign it to the formatted string
            ptr->Assign(result_string);

            vm::Value res;
            // assign register value to the allocated object
            res.m_type = vm::Value::HEAP_POINTER;
            res.m_value.ptr = ptr;

            ACE_RETURN(res);
        } else {
            params.handler->state->ThrowException(params.handler->thread, e);
        }
    } else {
        params.handler->state->ThrowException(params.handler->thread, e);
    }
}

void Global_array_push(ace::sdk::Params params)
{
    ACE_CHECK_ARGS(>=, 2);

    vm::Value *target_ptr = params.args[0];
    ASSERT(target_ptr != nullptr);

    const int buffer_size = 256;
    char buffer[buffer_size];
    std::snprintf(
        buffer,
        buffer_size,
        "array_push() requires an Array"
    );
    vm::Exception e = vm::Exception(utf::Utf8String(buffer));

    if (target_ptr->GetType() == vm::Value::ValueType::HEAP_POINTER) {
        vm::Array *array_ptr = nullptr;
        
        if (target_ptr->GetValue().ptr == nullptr) {
            params.handler->state->ThrowException(params.handler->thread, vm::Exception::NullReferenceException());
        } else if ((array_ptr = target_ptr->GetValue().ptr->GetPointer<vm::Array>()) != nullptr) {
            array_ptr->PushMany(params.nargs - 1, &params.args[1]);
        } else {
            params.handler->state->ThrowException(params.handler->thread, e);
        }
    } else {
        params.handler->state->ThrowException(params.handler->thread, e);
    }

    // return same value
    ACE_RETURN(*target_ptr);
}

void Global_length(ace::sdk::Params params)
{
    ACE_CHECK_ARGS(==, 1);

    int len = 0;

    vm::Value *target_ptr = params.args[0];
    ASSERT(target_ptr != nullptr);

    const int buffer_size = 256;
    char buffer[buffer_size];
    std::snprintf(
        buffer,
        buffer_size,
        "length() is undefined for type '%s'",
        target_ptr->GetTypeString()
    );
    vm::Exception e = vm::Exception(utf::Utf8String(buffer));

    if (target_ptr->GetType() == vm::Value::ValueType::HEAP_POINTER) {
        utf::Utf8String *str_ptr = nullptr;
        vm::Array *array_ptr = nullptr;
        vm::Object *obj_ptr = nullptr;
        
        if (target_ptr->GetValue().ptr == nullptr) {
            params.handler->state->ThrowException(params.handler->thread, vm::Exception::NullReferenceException());
        } else if ((str_ptr = target_ptr->GetValue().ptr->GetPointer<utf::Utf8String>()) != nullptr) {
            // get length of string
            len = str_ptr->GetLength();
        } else if ((array_ptr = target_ptr->GetValue().ptr->GetPointer<vm::Array>()) != nullptr) {
            // get length of array
            len = array_ptr->GetSize();
        } else if ((obj_ptr = target_ptr->GetValue().ptr->GetPointer<vm::Object>()) != nullptr) {
            // get number of members in object
            // first, get type
            const vm::TypeInfo *type_ptr = obj_ptr->GetTypePtr();
            ASSERT(type_ptr != nullptr);
            
            len = type_ptr->GetSize();
        } else {
            params.handler->state->ThrowException(params.handler->thread, e);
        }
    } else {
        params.handler->state->ThrowException(params.handler->thread, e);
    }

    vm::Value res;
    // assign register value to the length
    res.m_type = vm::Value::I32;
    res.m_value.i32 = len;

    ACE_RETURN(res);
}

void Global_call(ace::sdk::Params params)
{
    ACE_CHECK_ARGS(>=, 1);

    vm::Value *target_ptr = params.args[0];
    ASSERT(target_ptr != nullptr);

    // copy target, as args may change
    vm::Value target(*target_ptr);

    // call the function
    vm::VM::Invoke(
        params.handler,
        target,
        params.nargs - 1
    );
}

void Global_spawn_thread(ace::sdk::Params params)
{
    ACE_CHECK_ARGS(>=, 1);

    vm::Value *target_ptr = params.args[0];
    ASSERT(target_ptr != nullptr);

    vm::Value target(*target_ptr);

    const int buffer_size = 256;
    char buffer[buffer_size];
    std::snprintf(buffer, buffer_size, "spawn_thread() is undefined for type '%s'",
        target_ptr->GetTypeString());
    vm::Exception e = vm::Exception(utf::Utf8String(buffer));

    // the position of the bytecode stream before thread execution
    const size_t pos = params.handler->bs->Position();

    if (target.GetType() == vm::Value::ValueType::FUNCTION) {
        // create the thread
        vm::ExecutionThread *new_thread = params.handler->state->CreateThread();
        ASSERT(new_thread != nullptr);

        // copy values to the new stack
        for (int i = 1; i < params.nargs; i++) {
            if (params.args[i] != nullptr) {
                new_thread->GetStack().Push(*params.args[i]);
            }
        }
        ASSERT(params.handler != nullptr);
        ASSERT(params.handler->bs != nullptr);
        
        const vm::BytecodeStream bs_before = *params.handler->bs;
        vm::VMState *vm_state = params.handler->state;
        const size_t nargs = params.nargs;

        threads.emplace_back(std::thread([new_thread, bs_before, vm_state, target, nargs, pos] () {
            // create copy of byte stream
            vm::BytecodeStream newBs = bs_before;
            newBs.SetPosition(pos);

            vm::InstructionHandler instruction_handler(
                vm_state,
                new_thread,
                &newBs
            );

            // keep track of function depth so we can
            // quit the thread when the function returns
            const int func_depth_start = new_thread->m_func_depth;
            
            // call the function
            vm::VM::Invoke(
                &instruction_handler,
                target,
                nargs - 1
            );

            while (!newBs.Eof() && vm_state->good && (new_thread->m_func_depth - func_depth_start)) {
                uint8_t code;
                newBs.Read(&code, 1);

                vm_state->m_vm->HandleInstruction(
                    &instruction_handler,
                    code
                );
            }

            // remove the thread
            std::lock_guard<std::mutex> lock(mtx);
            vm_state->DestroyThread(new_thread->GetId());
        }));
    } else {
        params.handler->state->ThrowException(params.handler->thread, e);
    }
}

static void LexLine(const utf::Utf8String &line, TokenStream &token_stream)
{
    CompilationUnit tmp_compilation_unit;

    // send code to be compiled
    SourceFile source_file("<stdin>", line.GetBufferSize());
    std::memcpy(source_file.GetBuffer(), line.GetData(), line.GetBufferSize());
    SourceStream source_stream(&source_file);

    Lexer lex(source_stream, &token_stream, &tmp_compilation_unit);
    lex.Analyze();
}

static int RunBytecodeFile(
    vm::VM *vm,
    const utf::Utf8String &filename,
    bool record_time,
    int pos=0)
{
    ASSERT(vm != nullptr);

    threads.clear();

    // load bytecode from file
    std::ifstream file(filename.GetData(), std::ios::in | std::ios::binary | std::ios::ate);
    // file could not be opened
    if (!file.is_open()) {
        utf::cout << "Could not open file " << filename << "\n";
        return 1;
    }

    int64_t bytecode_size = file.tellg();
    file.seekg(0, std::ios::beg);

    ASSERT(bytecode_size > 0);

    char *bytecodes = new char[bytecode_size];
    file.read(bytecodes, bytecode_size);
    file.close();

    vm::BytecodeStream bytecode_stream(
        bytecodes,
        (size_t)bytecode_size,
        pos
    );

    // time how long execution took
    auto start = std::chrono::high_resolution_clock::now();

    vm->Execute(&bytecode_stream);

    if (record_time) {
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<
            std::chrono::duration<double, std::ratio<1>>
        >(end - start).count();
        utf::cout << "Elapsed time: " << elapsed_ms << "s\n";
    }

    // wait for threads
    for (std::thread &thread : threads) {
        thread.join();
    }
    threads.clear();

    delete[] bytecodes;

    return 0;
}

static int REPL(
    vm::VM *vm,
    CompilationUnit &compilation_unit,
    const utf::Utf8String &template_code,
    bool first_time = true)
{
    AstIterator ast_iterator;

    int indent = 0;
    int parentheses_counter = 0;
    int bracket_counter = 0;

    utf::Utf8String code;
    utf::Utf8String out_filename = "tmp.aex";

    if (first_time) {
        // truncate the file, overwriting any previous data
        std::ofstream out_file(out_filename.GetData(),
            std::ios::out | std::ios::binary | std::ios::trunc);
        out_file.close();

        code = template_code;
        utf::cout << code;

        // send code to be compiled
        SourceFile source_file("<stdin>", code.GetBufferSize());
        std::memcpy(source_file.GetBuffer(), code.GetData(), code.GetBufferSize());
        SourceStream source_stream(&source_file);

        TokenStream token_stream(TokenStreamInfo {
            source_file.GetFilePath()
        });

        Lexer lex(source_stream, &token_stream, &compilation_unit);
        lex.Analyze();

        Parser parser(&ast_iterator, &token_stream, &compilation_unit);
        parser.Parse(false);

        SemanticAnalyzer semantic_analyzer(&ast_iterator, &compilation_unit);
        semantic_analyzer.Analyze(false);
    }

    std::vector<utf::Utf8String> lines;
    std::string tmp_line;

    utf::cout << "> ";
    while (std::getline(std::cin, tmp_line)) {
        utf::Utf8String current_line(tmp_line.c_str());

        if (current_line == "quit" || current_line == "quit()") {
            break;
        }

        for (char ch : tmp_line) {
            if (ch == '{') {
                indent++;
            } else if (ch == '}') {
                indent--;
            } else if (ch == '(') {
                parentheses_counter++;
            } else if (ch == ')') {
                parentheses_counter--;
            } else if (ch == '[') {
                bracket_counter++;
            } else if (ch == ']') {
                bracket_counter--;
            }
        }

        bool wait_for_next = false;
        bool cont_token = false;
        
        {
            // run lexer on entered line to determine
            // if we should keep reading input

            CompilationUnit tmp_compilation_unit;

            // send code to be compiled
            SourceFile source_file("<stdin>", current_line.GetBufferSize());
            source_file.ReadIntoBuffer(
                current_line.GetData(),
                current_line.GetBufferSize()
            );
            
            SourceStream source_stream(&source_file);

            TokenStream tmp_ts(TokenStreamInfo {
                source_file.GetFilePath()
            });

            Lexer tmp_lex(source_stream, &tmp_ts, &tmp_compilation_unit);
            tmp_lex.Analyze();

            cont_token = !tmp_ts.m_tokens.empty() && tmp_ts.m_tokens.back().IsContinuationToken();
            wait_for_next = indent > 0 ||
                    parentheses_counter > 0 ||
                    bracket_counter > 0 ||
                    cont_token;
        }

        lines.push_back(current_line);

        if (!wait_for_next) {
            // so we can rewind upon errors
            int old_pos = ast_iterator.GetPosition();
            int old_stack_record = compilation_unit.GetInstructionStream().GetStackSize();
            // store the number of identifiers in the global scope,
            // so we can remove them on error
            
            ASSERT(compilation_unit.GetCurrentModule() != nullptr);
            ASSERT(compilation_unit.GetCurrentModule()->m_scopes.TopNode() != nullptr);
            
            size_t num_identifiers = compilation_unit
                .GetCurrentModule()
                ->m_scopes.Top()
                .GetIdentifierTable()
                .GetIdentifiers()
                .size();

            utf::Utf8String lines_combined;
            for (size_t i = 0; i < lines.size(); i++) {
                lines_combined += lines[i] + "\n";
            }

            // send code to be compiled
            SourceFile source_file("<stdin>", lines_combined.GetBufferSize());
            source_file.ReadIntoBuffer(lines_combined.GetData(), lines_combined.GetBufferSize());
            SourceStream source_stream(&source_file);

            TokenStream token_stream(TokenStreamInfo {
                source_file.GetFilePath()
            });

            Lexer lex(source_stream, &token_stream, &compilation_unit);
            lex.Analyze();

            Parser parser(&ast_iterator, &token_stream, &compilation_unit);
            parser.Parse(false);

            // in REPL mode only analyze if parsing and lexing went okay.
            SemanticAnalyzer semantic_analyzer(&ast_iterator, &compilation_unit);
            semantic_analyzer.Analyze(false);

            compilation_unit.GetErrorList().SortErrors();
            compilation_unit.GetErrorList().WriteOutput(utf::cout);

            if (!compilation_unit.GetErrorList().HasFatalErrors()) {
                // only optimize if there were no errors
                // before this point
                ast_iterator.SetPosition(old_pos);
                Optimizer optimizer(&ast_iterator, &compilation_unit);
                optimizer.Optimize(false);

                // compile into bytecode instructions
                ast_iterator.SetPosition(old_pos);
                Compiler compiler(&ast_iterator, &compilation_unit);
                compiler.Compile(false);

                // get active register
                int active_reg = compilation_unit.GetInstructionStream().GetCurrentRegister();
                
                // emit bytecode instructions to file
                std::ofstream temp_bytecode_file(out_filename.GetData(),
                    std::ios::out | std::ios::binary | std::ios::app | std::ios::ate);

                int64_t file_pos = temp_bytecode_file.tellp();

                if (!temp_bytecode_file.is_open()) {
                    utf::cout << "Could not open file for writing: " << out_filename << "\n";
                    for (int i = old_pos; i < ast_iterator.GetPosition(); i++) {
                        ast_iterator.Pop();
                    }
                    ast_iterator.SetPosition(old_pos);
                } else {
                    int64_t bytecode_file_size;

                    temp_bytecode_file << compilation_unit.GetInstructionStream();
                    compilation_unit.GetInstructionStream().ClearInstructions();
                    bytecode_file_size = temp_bytecode_file.tellp();
                    temp_bytecode_file.close();

                    // check if we even have to run the bytecode file
                    if (bytecode_file_size - file_pos > 0) {
                        ASSERT(vm->GetState().GetNumThreads() > 0);
                        vm::ExecutionThread *main_thread = vm->GetState().m_threads[0];
                        ASSERT(main_thread != nullptr);

                        // store stack size so that we can revert on error
                        size_t stack_size_before = main_thread->GetStack().GetStackPointer();

                        RunBytecodeFile(vm, out_filename, false, file_pos);

                        utf::cout << termcolor::green;
                        // print whatever is in active_reg
                        vm->Print(main_thread->GetRegisters()[active_reg]);
                        utf::cout << termcolor::reset;
                        utf::printf(UTF8_CSTR("\n"));

                        if (!vm->GetState().good) {
                            // if an exception was unhandled go back to previous state
                            // overwrite the bytecode file with the code that has been generated up to the point of error
                            // start by loading it into a temporary buffer
                            std::ifstream tmp_is(
                                out_filename.GetData(),
                                std::ios::in | std::ios::binary | std::ios::ate
                            );

                            // len is only the amount of bytes up to where we were before in the file.
                            int64_t len = std::min((int64_t)tmp_is.tellg(), file_pos);
                            // create buffer
                            char *buf = new char[len];
                            // seek to beginning
                            tmp_is.seekg(0, std::ios::beg);
                            // read into buffer
                            tmp_is.read(buf, len);
                            // close temporary file
                            tmp_is.close();

                            // create the new file to write the buffer to
                            std::ofstream tmp_os(
                                out_filename.GetData(),
                                std::ios::out | std::ios::binary
                            );
                            tmp_os.write(buf, len);
                            tmp_os.close();

                            // delete buffer, it's no longer needed
                            delete[] buf;

                            size_t stack_size_now = main_thread->GetStack().GetStackPointer();
                            main_thread->GetStack().Pop(stack_size_now - stack_size_before);
                            stack_size_now = stack_size_before;

                            // kill off any thread that isn't the main thread
                            for (int i = 1; i < vm->GetState().GetNumThreads(); i++) {
                                if (vm->GetState().m_threads[i] != nullptr) {
                                    vm->GetState().DestroyThread(i);
                                }
                            }

                            // trigger the GC after popping items from stack,
                            // to clean up any heap variables no longer in use.
                            main_thread->GetStack().MarkAll();
                            vm->GetState().GetHeap().Sweep();

                            // clear vm exception state
                            main_thread->GetExceptionState().Reset();

                            // we're good to go now
                            vm->GetState().good = true;

                            // undo compilation
                            compilation_unit.GetInstructionStream().SetStackSize(old_stack_record);
                            for (int i = old_pos; i < ast_iterator.GetPosition(); i++) {
                                ast_iterator.Pop();
                            }
                            ast_iterator.SetPosition(old_pos);

                            // restart the script
                            return REPL(vm, compilation_unit, "", false);
                        } else {
                            // everything is good, no compile or runtime errors here
                            // store the line
                            code += lines_combined;
                        }
                    }
                }
            } else {
                // remove the identifiers that were since declared
                ASSERT(compilation_unit.GetCurrentModule() != nullptr);
                ASSERT(compilation_unit.GetCurrentModule()->m_scopes.TopNode() != nullptr);
                
                IdentifierTable &tbl = compilation_unit
                    .GetCurrentModule()
                    ->m_scopes.Top()
                    .GetIdentifierTable();

                const size_t diff = tbl.GetIdentifiers().size() - num_identifiers;

                for (size_t i = 0; i < diff; i++) {
                    tbl.PopIdentifier();
                }
                
                for (int i = old_pos; i < ast_iterator.GetPosition(); i++) {
                    ast_iterator.Pop();
                }
                ast_iterator.SetPosition(old_pos);
                compilation_unit.GetErrorList().ClearErrors();
            }

            lines.clear();

            utf::cout << "> ";
        } else {
            if (indent) {
                for (int i = 0; i < indent; i++) {
                    utf::cout << "  ";
                }
            } else {
                if (parentheses_counter || bracket_counter || cont_token) {
                    utf::cout << "  ";
                }
            }
            utf::cout << "  ";
        }
    }

    // delete temporary bytecode file
    if (std::remove(out_filename.GetData())) {
        utf::cout << "Error deleting file " << out_filename << "\n";
    }

    return 0;
}

void HandleArgs(
    int argc,
    char *argv[],
    vm::VM &vm,
    CompilationUnit &compilation_unit,
    APIInstance &api)
{
    if (argc == 1) {
        // trigger the REPL when no command line arguments have been provided
        // disable static objects, so we can append the bytecode files.
        ace::compiler::Config::use_static_objects = false;
        // do not cull unused objects
        ace::compiler::Config::cull_unused_objects = false;

        REPL(&vm, compilation_unit, "// type 'quit' to exit\n");
    } else if (argc >= 2) {
        enum {
            COMPILE_SOURCE,
            DECOMPILE_BYTECODE,
            RUN_BYTECODE
        } mode = COMPILE_SOURCE;

        utf::Utf8String src_filename;
        utf::Utf8String out_filename;

        bool native_mode = false;
        
        if (CLI::HasOption(argv, argv + argc, "cppgen")) {
            native_mode = true;
        }

        if (CLI::HasOption(argv, argv + argc, "-d")) {
            // disassembly mode
            mode = DECOMPILE_BYTECODE;
            src_filename = CLI::GetOptionValue(argv, argv + argc, "-d");

            if (CLI::HasOption(argv, argv + argc, "-o")) {
                out_filename = CLI::GetOptionValue(argv, argv + argc, "-o");
            }
        } else if (CLI::HasOption(argv, argv + argc, "-b")) {
            mode = RUN_BYTECODE;
            src_filename = CLI::GetOptionValue(argv, argv + argc, "-b");
        } else {
            mode = COMPILE_SOURCE;

            if (CLI::HasOption(argv, argv + argc, "-c")) {
                src_filename = CLI::GetOptionValue(argv, argv + argc, "-c");
            }

            if (src_filename == "") {
                src_filename = argv[argc - 1];
            }

            if (CLI::HasOption(argv, argv + argc, "-o")) {
                out_filename = CLI::GetOptionValue(argv, argv + argc, "-o");
            }

            if (out_filename == "") {
                out_filename = (str_util::strip_extension(src_filename.GetData()) + ".aex").c_str();
            }
        }

        if (mode == COMPILE_SOURCE) {
            if (ace_compiler::BuildSourceFile(src_filename, out_filename, compilation_unit)) {
                if (native_mode) {
                    // generate cpp code file
                    CppGenerator cppgen;

                    cppgen << compilation_unit.GetInstructionStream();
                    utf::cout << "Gen cpp: \n" << cppgen << "\n";

                } else {
                    // emit bytecode instructions to file
                    std::ofstream out_file(out_filename.GetData(),
                        std::ios::out | std::ios::binary);
                    
                    if (!out_file.is_open()) {
                        utf::cout << "Could not open file for writing: " << out_filename << "\n";
                    } else {
                        out_file << compilation_unit.GetInstructionStream();
                    }

                    out_file.close();

                    // execute the compiled bytecode file
                    RunBytecodeFile(&vm, out_filename, true);
                }
            }
        } else if (mode == DECOMPILE_BYTECODE) {
            ace_compiler::DecompileBytecodeFile(src_filename, out_filename);
        } else if (mode == RUN_BYTECODE) {
            RunBytecodeFile(&vm, src_filename, true);
        }
    }
}

void BuildLibraries(
    vm::VM &vm,
    CompilationUnit &compilation_unit,
    APIInstance &api)
{
    api.Module("events")
        .Function("get_action_handler", SymbolType::Builtin::FUNCTION, {
            { "event_array", SymbolType::Builtin::ANY },
            { "match", SymbolType::Builtin::ANY }
        }, Events_get_action_handler)
        .Function("call_action", SymbolType::Builtin::ANY, {
            { "obj", SymbolType::Builtin::ANY },
            { "key", SymbolType::Builtin::ANY },
            { "args", SymbolType::GenericInstance(
                SymbolType::Builtin::VAR_ARGS,
                GenericInstanceTypeInfo {
                    { { "arg", SymbolType::Builtin::ANY } }
                }
            ) }
        }, Events_call_action);

    api.Module("time")
        .Function("now", SymbolType::Builtin::INT, {}, Time_now);

    api.Module("runtime")
        .Function("gc", SymbolType::Builtin::NULL_TYPE, {}, Runtime_gc)
        .Function("dump_heap", SymbolType::Builtin::NULL_TYPE, {}, Runtime_dump_heap)
        .Function("dump_stack", SymbolType::Builtin::NULL_TYPE, {}, Runtime_dump_stack)
        .Function("typeof", SymbolType::Builtin::STRING, {
            { "object", SymbolType::Builtin::ANY }
        }, Runtime_typeof)
        .Function("load_library", SymbolType::Builtin::ANY, {
            { "path", SymbolType::Builtin::STRING }
        }, Runtime_load_library)
        .Function("load_function", SymbolType::Builtin::FUNCTION, {
            { "lib", SymbolType::Builtin::ANY },
            { "function_name", SymbolType::Builtin::STRING }
        }, Runtime_load_function)
        .Variable(
            "version",
            SymbolType::Builtin::ARRAY,
            [](vm::VMState *state, vm::ExecutionThread *thread, vm::Value *out) {
                ASSERT(state != nullptr);
                ASSERT(out != nullptr);

                // allocate heap value
                vm::HeapValue *hv = state->HeapAlloc(thread);
                ASSERT(hv != nullptr);

                // create each version object
                vm::Value sv_major;
                sv_major.m_type = vm::Value::ValueType::I32;
                sv_major.m_value.i32 = Runtime::VERSION_MAJOR;

                vm::Value sv_minor;
                sv_minor.m_type = vm::Value::ValueType::I32;
                sv_minor.m_value.i32 = Runtime::VERSION_MINOR;

                vm::Value sv_patch;
                sv_patch.m_type = vm::Value::ValueType::I32;
                sv_patch.m_value.i32 = Runtime::VERSION_PATCH;

                // create array
                vm::Array res(3);
                res.AtIndex(0) = sv_major;
                res.AtIndex(1) = sv_minor;
                res.AtIndex(2) = sv_patch;

                // assign heap value to array
                hv->Assign(res);

                // assign the out value to this
                out->m_type = vm::Value::ValueType::HEAP_POINTER;
                out->m_value.ptr = hv;
            }
        )
        .Variable(
            "os_name",
            SymbolType::Builtin::STRING,
            [](vm::VMState *state, vm::ExecutionThread *thread, vm::Value *out) {
                ASSERT(state != nullptr);
                ASSERT(out != nullptr);

                // allocate heap value
                vm::HeapValue *hv = state->HeapAlloc(thread);
                ASSERT(hv != nullptr);

                // create string and set to to hold the name
                utf::Utf8String res = Runtime::OS_NAME;

                // assign heap value to array
                hv->Assign(res);

                // assign the out value to this
                out->m_type = vm::Value::ValueType::HEAP_POINTER;
                out->m_value.ptr = hv;
            }
        );

    api.Module(ace::compiler::Config::global_module_name)
        /*.Type(SymbolType::Object("Range", {
            {
                "step",
                SymbolType::Builtin::NUMBER,
                std::shared_ptr<AstInteger>(new AstInteger(
                    1, SourceLocation::eof
                ))
            },
            {
                "first",
                SymbolType::Builtin::NUMBER,
                std::shared_ptr<AstInteger>(new AstInteger(
                    0, SourceLocation::eof
                ))
            },
            {
                "last",
                SymbolType::Builtin::NUMBER,
                std::shared_ptr<AstInteger>(new AstInteger(
                    10, SourceLocation::eof
                ))
            }
        }))*/
        .Function("prompt", SymbolType::Builtin::STRING, {
            { "message", SymbolType::Builtin::STRING }
        }, Global_prompt)
        .Function("to_string", SymbolType::Builtin::STRING, {
            { "object", SymbolType::Builtin::ANY },
        }, Global_to_string)
        .Function("decompile", SymbolType::Builtin::STRING, {
            { "f", SymbolType::Builtin::FUNCTION },
        }, Global_decompile)
        .Function("fmt", SymbolType::Builtin::STRING, {
            { "format", SymbolType::Builtin::STRING },
            { "args", SymbolType::GenericInstance(
                SymbolType::Builtin::VAR_ARGS,
                GenericInstanceTypeInfo {
                    {
                        { "arg", SymbolType::Builtin::ANY }
                    }
                }
            ) }
        }, Global_fmt)
        .Function("to_json", SymbolType::Builtin::STRING, {
            { "object", SymbolType::Builtin::ANY }
        }, Global_to_json)
        .Function("array_push", SymbolType::Builtin::ARRAY, {
            { "arr", SymbolType::Builtin::ARRAY },
            { "args", SymbolType::GenericInstance(
                SymbolType::Builtin::VAR_ARGS,
                GenericInstanceTypeInfo {
                    {
                        { "arg", SymbolType::Builtin::ANY }
                    }
                }
            ) }
        }, Global_array_push)
        .Function("length", SymbolType::Builtin::INT, {
            { "object", SymbolType::Builtin::ANY }
        }, Global_length)
        .Function("call", SymbolType::Builtin::ANY, {
            { "f", SymbolType::Builtin::FUNCTION },
            { "args", SymbolType::GenericInstance(
                SymbolType::Builtin::VAR_ARGS,
                GenericInstanceTypeInfo {
                    {
                        { "arg", SymbolType::Builtin::ANY }
                    }
                }
            ) }
        }, Global_call)
        .Function("spawn_thread", SymbolType::Builtin::ANY, {
            { "f", SymbolType::Builtin::FUNCTION },
            { "args", SymbolType::GenericInstance(
                SymbolType::Builtin::VAR_ARGS,
                GenericInstanceTypeInfo {
                    {
                        { "arg", SymbolType::Builtin::ANY }
                    }
                }
            ) }
        }, Global_spawn_thread);

    api.BindAll(&vm, &compilation_unit);
}

int main(int argc, char *argv[])
{
    utf::init();

    if (argc >= 1) {
        exec_path = argv[0];
        // trim the exec name
        size_t index = exec_path.find_last_of("/\\");
        if (index != std::string::npos) {
            exec_path = exec_path.substr(0, index) + "/";
        }
    }

    vm::VM vm;
    CompilationUnit compilation_unit;
    APIInstance api;

    BuildLibraries(
        vm,
        compilation_unit,
        api
    );

    HandleArgs(
        argc,
        argv,
        vm,
        compilation_unit,
        api
    );
}
