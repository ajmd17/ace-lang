#ifndef META_HPP
#define META_HPP

namespace ace {
namespace vm {
    class VM;
}

class APIInstance;

}

class CompilationUnit;

class Meta {
public:
    static void BuildMetaLibrary(ace::vm::VM &vm,
        CompilationUnit &compilation_unit,
        ace::APIInstance &api);
};

#endif