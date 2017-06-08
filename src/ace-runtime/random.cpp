// Ace Random library

#include <ace-sdk/ace-sdk.hpp>

#include <ace-vm/VMState.hpp>
#include <ace-vm/InstructionHandler.hpp>
#include <ace-vm/Exception.hpp>

#include <random>

using namespace ace;

ACE_FUNCTION(random_new_random)
{
    ACE_CHECK_ARGS(==, 1);

    const vm::Value *target_ptr = params.args[0];
    ASSERT(target_ptr != nullptr);

    ace::aint64 seed;
    if (target_ptr->GetInteger(&seed)) {
        std::mt19937_64 gen;
        gen.seed(seed);

        // create heap value for random generator
        vm::HeapValue *ptr = params.handler->state->HeapAlloc(params.handler->thread);
        ASSERT(ptr != nullptr);
        ptr->Assign(gen);

        // assign register value to the allocated object
        vm::Value res;
        res.m_type = vm::Value::HEAP_POINTER;
        res.m_value.ptr = ptr;

        ACE_RETURN(res);
    } else {
        params.handler->state->ThrowException(
            params.handler->thread,
            vm::Exception("invalid seed value")
        );
    }
}

ACE_FUNCTION(random_get_next)
{
    ACE_CHECK_ARGS(==, 1);

    const vm::Value *target_ptr = params.args[0];
    ASSERT(target_ptr != nullptr);

    if (std::mt19937_64 *gen_ptr = target_ptr->GetValue().ptr->GetPointer<std::mt19937_64>()) {
        ace::aint64 value = (*gen_ptr)();

        // assign register value to the generated value
        vm::Value res;
        res.m_type = vm::Value::ValueType::I64;
        res.m_value.i64 = value;

        ACE_RETURN(res);
    } else {
        params.handler->state->ThrowException(
            params.handler->thread,
            vm::Exception("invalid random generator")
        );
    }
}

ACE_FUNCTION(random_crand)
{
    ACE_CHECK_ARGS(==, 0);

    int value = std::rand();

    // assign register value to the generated value
    vm::Value res;
    res.m_type = vm::Value::ValueType::I32;
    res.m_value.i32 = value;

    ACE_RETURN(res);
}