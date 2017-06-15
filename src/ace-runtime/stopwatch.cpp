// Ace stopwatch library

#include <ace-sdk/ace-sdk.hpp>

#include <ace-vm/VMState.hpp>
#include <ace-vm/InstructionHandler.hpp>
#include <ace-vm/Exception.hpp>

#include <common/timer.hpp>

#include <cstdio>

using namespace ace;

static Timer stopwatch;

ACE_FUNCTION(stopwatch_start)
{
    ACE_CHECK_ARGS(==, 0);

    // start timer
    if (std::int64_t now = stopwatch.Start()) {
        // return the timestamp
        vm::Value res;
        res.m_type = vm::Value::ValueType::I64;
        res.m_value.i64 = now;
        ACE_RETURN(res);
    } else {
        params.handler->state->ThrowException(
            params.handler->thread,
            vm::Exception("Failed to start stopwatch")
        );
    }
}

ACE_FUNCTION(stopwatch_stop)
{
    ACE_CHECK_ARGS(==, 0);

    double elapsed = stopwatch.Elapsed();

    // return the elapsed time
    vm::Value res;
    res.m_type = vm::Value::ValueType::F64;
    res.m_value.d = elapsed;
    ACE_RETURN(res);
}