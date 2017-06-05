#include <ace-c/dis/DecompilationUnit.hpp>

#include <common/instructions.hpp>

#include <sstream>
#include <cstring>
#include <cstdio>

DecompilationUnit::DecompilationUnit()
{
}

void DecompilationUnit::DecodeNext(
    uint8_t code,
    ace::vm::BytecodeStream &bs,
    InstructionStream &is,
    utf::utf8_ostream *os)
{

    switch (code) {
    case NOP:
    {
        if (os != nullptr) {
            (*os) << "nop" << std::endl;
        }

        is << Instruction<uint8_t>(code);

        break;
    }
    case STORE_STATIC_STRING:
    {
        uint32_t len;
        bs.Read(&len);

        char *str = new char[len + 1];
        str[len] = '\0';
        bs.Read(str, len);

        if (os != nullptr) {
            (*os)
                << "str ["
                    << "u32(" << len << "), "
                    << "\"" << str << "\""
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint32_t, const char*>(code, len, str);

        delete[] str;

        break;
    }
    case STORE_STATIC_ADDRESS:
    {
        uint32_t val;
        bs.Read(&val);

        if (os != nullptr) {
            
            (*os) << "addr [@(" << std::hex << val << std::dec << ")]" << std::endl;
        }

        is << Instruction<uint8_t, uint32_t>(code, val);

        break;
    }
    case STORE_STATIC_FUNCTION:
    {
        uint32_t addr;
        bs.Read(&addr);

        uint8_t nargs;
        bs.Read(&nargs);

        uint8_t flags;
        bs.Read(&flags);

        if (os != nullptr) {
            
            (*os) << "function [@(" << std::hex << addr << std::dec << "), "
                    << "u8(" << (int)nargs << ")], "
                    << "u8(" << (int)flags << ")]"
                    << std::endl;
        }

        is << Instruction<uint8_t, uint32_t, uint8_t, uint8_t>(code, addr, nargs, flags);

        break;
    }
    case STORE_STATIC_TYPE:
    {
        uint16_t type_name_len;
        bs.Read(&type_name_len);

        std::vector<uint8_t> type_name;
        type_name.resize(type_name_len + 1);
        type_name[type_name_len] = '\0';
        bs.Read(&type_name[0], type_name_len);

        uint16_t size;
        bs.Read(&size);

        std::vector<std::vector<uint8_t>> names;
        names.resize(size);
        
        for (int i = 0; i < size; i++) {
            uint16_t len;
            bs.Read(&len);

            names[i].resize(len + 1);
            names[i][len] = '\0';
            bs.Read(&names[i][0], len);
        }

        if (os != nullptr) {
            (*os)
                << "type ["
                    << "str(" << type_name.data() << "), "
                    << "u16(" << (int)size << "), ";

            for (int i = 0; i < size; i++) {
                (*os)
                    << "str(" << names[i].data() << ")";
                if (i != size - 1) {
                    (*os) << ", ";
                }
            }
                    
            (*os)
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, std::vector<uint8_t>, uint16_t, std::vector<std::vector<uint8_t>>>
            (code, type_name, size, names);

        break;
    }
    case LOAD_I32:
    {
        uint8_t reg;
        bs.Read(&reg);

        int32_t val;
        bs.Read(&val);

        if (os != nullptr) {
            (*os)
                << "load_i32 ["
                    << "%" << (int)reg << ", "
                    << "i32(" << val << ")"
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t, int32_t>(code, reg, val);

        break;
    }
    case LOAD_I64:
    {
        uint8_t reg;
        bs.Read(&reg);

        int64_t val;
        bs.Read(&val);

        if (os != nullptr) {
            (*os)
                << "load_i64 ["
                    << "%" << (int)reg << ", "
                    << "i64(" << val << ")"
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t, int64_t>(code, reg, val);

        break;
    }
    case LOAD_F32:
    {
        uint8_t reg;
        bs.Read(&reg);

        float val;
        bs.Read(&val);

        if (os != nullptr) {
            (*os)
                << "load_f32 ["
                    << "%" << (int)reg << ", "
                    << "f32(" << val << ")"
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t, float>(code, reg, val);

        break;
    }
    case LOAD_F64:
    {
        uint8_t reg;
        bs.Read(&reg);

        double val;
        bs.Read(&val);

        if (os != nullptr) {
            (*os)
                << "load_f64 ["
                    << "%" << (int)reg << ", "
                    << "f64(" << val << ")"
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t, double>(code, reg, val);

        break;
    }
    case LOAD_OFFSET:
    {
        uint8_t reg;
        bs.Read(&reg);

        uint16_t offset;
        bs.Read(&offset);

        if (os != nullptr) {
            (*os)
                << "load_offset ["
                    << "%" << (int)reg << ", "
                    "$(sp-" << offset << ")"
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t, uint16_t>(code, reg, offset);

        break;
    }
    case LOAD_INDEX:
    {
        uint8_t reg;
        bs.Read(&reg);

        uint16_t idx;
        bs.Read(&idx);

        if (os != nullptr) {
            (*os)
                << "load_index ["
                    << "%" << (int)reg << ", "
                    "u16(" << idx << ")"
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t, uint16_t>(code, reg, idx);

        break;
    }
    case LOAD_STATIC:
    {
        uint8_t reg;
        bs.Read(&reg);

        uint16_t index;
        bs.Read(&index);

        if (os != nullptr) {
            (*os)
                << "load_static ["
                    << "%" << (int)reg << ", "
                    << "#" << index
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t, uint16_t>(code, reg, index);

        break;
    }
    case LOAD_STRING:
    {
        uint8_t reg;
        bs.Read(&reg);

        // get string length
        uint32_t len;
        bs.Read(&len);

        // read string based on length
        char *str = new char[len + 1];
        bs.Read(str, len);
        str[len] = '\0';

        if (os != nullptr) {
            (*os)
                << "load_str ["
                    << "%" << (int)reg << ", "
                    << "u32(" << len << "), "
                    << "\"" << str << "\""
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t, uint32_t, const char*>(code, reg, len, str);

        delete[] str;

        break;
    }
    case LOAD_ADDR:
    {
        uint8_t reg;
        bs.Read(&reg);

        uint32_t val;
        bs.Read(&val);

        if (os != nullptr) {
            
            (*os) << "load_addr [%" << (int)reg << ", @(" << std::hex << val << std::dec << ")]" << std::endl;
        }

        is << Instruction<uint8_t, uint8_t, uint32_t>(code, reg, val);

        break;
    }
    case LOAD_FUNC:
    {
        uint8_t reg;
        bs.Read(&reg);

        uint32_t addr;
        bs.Read(&addr);

        uint8_t nargs;
        bs.Read(&nargs);

        uint8_t flags;
        bs.Read(&flags);

        if (os != nullptr) {
            (*os) << "load_func [%" << (int)reg
                    << ", @(" << std::hex << addr << std::dec << "), "
                    << "u8(" << (int)nargs << ")], "
                    << "u8(" << (int)flags << ")]"
                    << std::endl;
        }

        is << Instruction<uint8_t, uint8_t, uint32_t, uint8_t, uint8_t>(code, reg, addr, nargs, flags);

        break;
    }
    case LOAD_TYPE:
    {
        uint8_t reg;
        bs.Read(&reg);

        uint16_t type_name_len;
        bs.Read(&type_name_len);

        std::vector<uint8_t> type_name;
        type_name.resize(type_name_len + 1);
        type_name[type_name_len] = '\0';
        bs.Read(&type_name[0], type_name_len);

        uint16_t size;
        bs.Read(&size);

        std::vector<std::vector<uint8_t>> names;
        names.resize(size);
        
        for (int i = 0; i < size; i++) {
            uint16_t len;
            bs.Read(&len);

            names[i].resize(len + 1);
            names[i][len] = '\0';
            bs.Read(&names[i][0], len);
        }

        if (os != nullptr) {
            (*os)
                << "load_type ["
                    << "%" << (int)reg << ", "
                    << "str(" << type_name.data() << ")"
                    << "u16(" << (int)size << "), ";

            for (int i = 0; i < size; i++) {
                (*os)
                    << "str(" << names[i].data() << ")";
                if (i != size - 1) {
                    (*os) << ", ";
                }
            }
                    
            (*os)
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t, std::vector<uint8_t>, uint16_t, std::vector<std::vector<uint8_t>>>
            (code, reg, type_name, size, names);

        break;
    }
    case LOAD_MEM:
    {
        uint8_t reg;
        bs.Read(&reg);

        uint8_t src;
        bs.Read(&src);

        uint8_t idx;
        bs.Read(&idx);

        if (os != nullptr) {
            (*os)
                << "load_mem ["
                    << "%" << (int)reg << ", "
                    << "%" << (int)src << ", "
                    << "u8(" << (int)idx << ")"
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t, uint8_t, uint8_t>(code, reg, src, idx);

        break;
    }
    case LOAD_MEM_HASH:
    {
        uint8_t reg;
        bs.Read(&reg);

        uint8_t src;
        bs.Read(&src);

        uint32_t hash;
        bs.Read(&hash);

        if (os != nullptr) {
            (*os)
                << "load_mem_hash ["
                    << "%" << (int)reg << ", "
                    << "%" << (int)src << ", "
                    << "u32(" << hash << ")"
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t, uint8_t, uint32_t>(code, reg, src, hash);

        break;
    }
    case LOAD_ARRAYIDX:
    {
        uint8_t reg;
        bs.Read(&reg);

        uint8_t src;
        bs.Read(&src);

        uint8_t idx;
        bs.Read(&idx);

        if (os != nullptr) {
            (*os)
                << "load_arrayidx ["
                    << "%" << (int)reg << ", "
                    << "%" << (int)src << ", "
                    << "%" << (int)idx << ")"
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t, uint8_t, uint8_t>(code, reg, src, idx);

        break;
    }
    case LOAD_NULL:
    {
        uint8_t reg;
        bs.Read(&reg);

        if (os != nullptr) {
            (*os)
                << "load_null ["
                    << "%" << (int)reg
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t>(code, reg);

        break;
    }
    case LOAD_TRUE:
    {
        uint8_t reg;
        bs.Read(&reg);

        if (os != nullptr) {
            (*os)
                << "load_true ["
                    << "%" << (int)reg
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t>(code, reg);

        break;
    }
    case LOAD_FALSE:
    {
        uint8_t reg;
        bs.Read(&reg);

        if (os != nullptr) {
            (*os)
                << "load_false ["
                    << "%" << (int)reg
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t>(code, reg);

        break;
    }
    case MOV_OFFSET:
    {
        uint16_t dst;
        bs.Read(&dst);

        uint8_t src;
        bs.Read(&src);

        if (os != nullptr) {
            (*os)
                << "mov_offset ["
                    << "$(sp-" << dst << "), "
                    << "%" << (int)src
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint16_t, uint8_t>(code, dst, src);

        break;
    }
    case MOV_INDEX:
    {
        uint16_t dst;
        bs.Read(&dst);

        uint8_t src;
        bs.Read(&src);

        if (os != nullptr) {
            (*os)
                << "mov_index ["
                    << "u16(" << dst << "), "
                    << "%" << (int)src
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint16_t, uint8_t>(code, dst, src);

        break;
    }
    case MOV_MEM:
    {
        uint8_t reg;
        bs.Read(&reg);

        uint8_t idx;
        bs.Read(&idx);

        uint8_t src;
        bs.Read(&src);

        if (os != nullptr) {
            (*os)
                << "mov_mem ["
                    << "%" << (int)reg << ", "
                    << "u8(" << (int)idx << "), "
                    << "%" << (int)src << ""
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t, uint8_t, uint8_t>(code, reg, idx, src);

        break;
    }
    case MOV_MEM_HASH:
    {
        uint8_t reg;
        bs.Read(&reg);

        uint32_t hash;
        bs.Read(&hash);

        uint8_t src;
        bs.Read(&src);

        if (os != nullptr) {
            (*os)
                << "mov_mem_hash ["
                    << "%" << (int)reg << ", "
                    << "u32(" << hash << "), "
                    << "%" << (int)src
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t, uint32_t, uint8_t>(code, reg, hash, src);

        break;
    }
    case MOV_ARRAYIDX:
    {
        uint8_t reg;
        bs.Read(&reg);

        uint32_t idx;
        bs.Read(&idx);

        uint8_t src;
        bs.Read(&src);

        if (os != nullptr) {
            (*os)
                << "mov_arrayidx ["
                    << "%" << (int)reg << ", "
                    << "u32(" << (int)idx << "), "
                    << "%" << (int)src << ""
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t, uint32_t, uint8_t>(code, reg, idx, src);

        break;
    }
    case MOV_REG:
    {
        uint8_t dst;
        bs.Read(&dst);

        uint8_t src;
        bs.Read(&src);

        if (os != nullptr) {
            (*os)
                << "mov_reg ["
                    << "%" << (int)dst << ", "
                    << "%" << (int)src << ""
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t, uint8_t>(code, dst, src);

        break;
    }
    case HAS_MEM_HASH:
    {
        uint8_t reg;
        bs.Read(&reg);

        uint8_t src;
        bs.Read(&src);

        uint32_t hash;
        bs.Read(&hash);

        if (os != nullptr) {
            (*os)
                << "has_mem_hash ["
                    << "%" << (int)reg << ", "
                    << "%" << (int)src << ", "
                    << "u32(" << hash << ")"
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t, uint8_t, uint32_t>(code, reg, src, hash);

        break;
    }
    case PUSH:
    {
        uint8_t src;
        bs.Read(&src);

        if (os != nullptr) {
            (*os)
                << "push ["
                    << "%" << (int)src
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t>(code, src);

        break;
    }
    case POP:
    {
        if (os != nullptr) {
            (*os)
                << "pop"
                << std::endl;
        }

        is << Instruction<uint8_t>(code);

        break;
    }
    case POP_N:
    {
        uint8_t n;
        bs.Read(&n);

        if (os != nullptr) {
            (*os)
                << "pop_n ["
                << "u8(" << (int)n << ")"
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t>(code);

        break;
    }
    case PUSH_ARRAY:
    {
        uint8_t dst;
        bs.Read(&dst);

        uint8_t src;
        bs.Read(&src);

        if (os != nullptr) {
            (*os)
                << "push_array ["
                << "% " << (int)dst << ", "
                << "% " << (int)src
                << "]" << std::endl;
        }

        is << Instruction<uint8_t, uint8_t, uint8_t>(code, dst, src);

        break;
    }
    case ECHO:
    {
        uint8_t reg;
        bs.Read(&reg);

        if (os != nullptr) {
            (*os)
                << "echo ["
                    << "%" << (int)reg
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t>(code, reg);

        break;
    }
    case ECHO_NEWLINE:
    {
        if (os != nullptr) {
            (*os)
                << "echo_newline"
                << std::endl;
        }

        is << Instruction<uint8_t>(code);

        break;
    }
    case JMP:
    {
        uint8_t addr;
        bs.Read(&addr);

        if (os != nullptr) {

            (*os)
                << "jmp ["
                    << "%" << (int)addr
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t>(code, addr);

        break;
    }
    case JE:
    {
        uint8_t addr;
        bs.Read(&addr);

        if (os != nullptr) {

            (*os)
                << "je ["
                    << "%" << (int)addr
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t>(code, addr);

        break;
    }
    case JNE:
    {
        uint8_t addr;
        bs.Read(&addr);

        if (os != nullptr) {

            (*os)
                << "jne ["
                    << "%" << (int)addr
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t>(code, addr);

        break;
    }
    case JG:
    {
        uint8_t addr;
        bs.Read(&addr);

        if (os != nullptr) {

            (*os)
                << "jg ["
                    << "%" << (int)addr
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t>(code, addr);

        break;
    }
    case JGE:
    {
        uint8_t addr;
        bs.Read(&addr);

        if (os != nullptr) {

            (*os)
                << "jge ["
                    << "%" << (int)addr
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t>(code, addr);

        break;
    }
    case CALL:
    {
        uint8_t func;
        bs.Read(&func);

        uint8_t argc;
        bs.Read(&argc);

        if (os != nullptr) {

            (*os)
                << "call ["
                    << "%" << (int)func << ", "
                    << "u8(" << (int)argc << ")"
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t, uint8_t>(code, func, argc);

        break;
    }
    case RET:
    {
        if (os != nullptr) {

            (*os)
                << "ret"
                << std::endl;
        }

        is << Instruction<uint8_t>(code);

        break;
    }
    case BEGIN_TRY:
    {
        uint8_t reg;
        bs.Read(&reg);

        if (os != nullptr) {

            (*os)
                << "begin_try ["
                    << "%" << (int)reg
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t>(code, reg);

        break;
    }
    case END_TRY:
    {
        if (os != nullptr) {

            (*os) << "end_try" << std::endl;
        }

        is << Instruction<uint8_t>(code);

        break;
    }
    case NEW:
    {
        uint8_t dst;
        bs.Read(&dst);

        uint8_t type;
        bs.Read(&type);

        if (os != nullptr) {

            (*os)
                << "new ["
                    << "%" << (int)dst << ", "
                    << "%" << (int)type
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t, uint8_t>(code, dst, type);

        break;
    }
    case NEW_ARRAY:
    {
        uint8_t dst;
        bs.Read(&dst);

        uint32_t size;
        bs.Read(&size);

        if (os != nullptr) {

            (*os)
                << "new_array ["
                    << "%"    << (int)dst << ", "
                    << "u32(" << (int)size << ")"
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t, uint32_t>(code, dst, size);

        break;
    }
    case CMP:
    {
        uint8_t lhs;
        bs.Read(&lhs);

        uint8_t rhs;
        bs.Read(&rhs);

        if (os != nullptr) {

            (*os)
                << "cmp ["
                    << "%" << (int)lhs << ", "
                    << "%" << (int)rhs
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t, uint8_t>(code, lhs, rhs);

        break;
    }
    case CMPZ:
    {
        uint8_t lhs;
        bs.Read(&lhs);

        if (os != nullptr) {

            (*os)
                << "cmpz ["
                    << "%" << (int)lhs
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t>(code, lhs);

        break;
    }
    case ADD:
    {
        uint8_t lhs;
        bs.Read(&lhs);

        uint8_t rhs;
        bs.Read(&rhs);

        uint8_t dst;
        bs.Read(&dst);

        if (os != nullptr) {

            (*os)
                << "add ["
                    << "%" << (int)lhs << ", "
                    << "%" << (int)rhs << ", "
                    << "%" << (int)dst
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t, uint8_t, uint8_t>(code, lhs, rhs, dst);

        break;
    }
    case SUB:
    {
        uint8_t lhs;
        bs.Read(&lhs);

        uint8_t rhs;
        bs.Read(&rhs);

        uint8_t dst;
        bs.Read(&dst);

        if (os != nullptr) {

            (*os)
                << "sub ["
                    << "%" << (int)lhs << ", "
                    << "%" << (int)rhs << ", "
                    << "%" << (int)dst
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t, uint8_t, uint8_t>(code, lhs, rhs, dst);

        break;
    }
    case MUL:
    {
        uint8_t lhs;
        bs.Read(&lhs);

        uint8_t rhs;
        bs.Read(&rhs);

        uint8_t dst;
        bs.Read(&dst);

        if (os != nullptr) {

            (*os)
                << "mul ["
                    << "%" << (int)lhs << ", "
                    << "%" << (int)rhs << ", "
                    << "%" << (int)dst
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t, uint8_t, uint8_t>(code, lhs, rhs, dst);

        break;
    }
    case DIV:
    {
        uint8_t lhs;
        bs.Read(&lhs);

        uint8_t rhs;
        bs.Read(&rhs);

        uint8_t dst;
        bs.Read(&dst);

        if (os != nullptr) {

            (*os)
                << "div ["
                    << "%" << (int)lhs << ", "
                    << "%" << (int)rhs << ", "
                    << "%" << (int)dst
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t, uint8_t, uint8_t>(code, lhs, rhs, dst);

        break;
    }
    case MOD:
    {
        uint8_t lhs;
        bs.Read(&lhs);

        uint8_t rhs;
        bs.Read(&rhs);

        uint8_t dst;
        bs.Read(&dst);

        if (os != nullptr) {

            (*os)
                << "mod ["
                    << "%" << (int)lhs << ", "
                    << "%" << (int)rhs << ", "
                    << "%" << (int)dst
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t, uint8_t, uint8_t>(code, lhs, rhs, dst);

        break;
    }
    case NEG:
    {
        uint8_t reg;
        bs.Read(&reg);

        if (os != nullptr) {

            (*os)
                << "neg ["
                    << "%" << (int)reg
                << "]"
                << std::endl;
        }

        is << Instruction<uint8_t, uint8_t>(code, reg);

        break;
    }
    case EXIT:
    {
        if (os != nullptr) {

            (*os)
                << "exit"
                << std::endl;
        }

        is << Instruction<uint8_t>(code);

        break;
    }
    default:
        if (os != nullptr) {

            (*os)
                << "??"
                << std::endl;
        }
        // unrecognized instruction
        is << Instruction<uint8_t>(code);

        break;
    }
}

InstructionStream DecompilationUnit::Decompile(ace::vm::BytecodeStream &bs, utf::utf8_ostream *os)
{
    InstructionStream is;

    while (!bs.Eof()) {
        const size_t pos = bs.Position();
        
        if (os != nullptr) {
            (*os) << std::hex << pos << std::dec << "\t";
        }

        uint8_t code;
        bs.Read(&code);

        DecodeNext(code, bs, is, os);
    }

    return is;
}
