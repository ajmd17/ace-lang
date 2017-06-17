#ifndef INSTRUCTION_OBJECT_HPP
#define INSTRUCTION_OBJECT_HPP

#include <ace-c/emit/NamesPair.hpp>

#include <vector>
#include <streambuf>

template<int Index, typename First, typename... Rest>
struct GetImpl;

template<typename First, typename... Rest>
struct GetImpl<0, First, Rest...>;

template<typename First, typename... Rest>
struct SizeImpl;

template<typename First>
struct SizeImpl<First>;

template<typename First, typename... Rest>
struct Builder;

template<typename First>
struct Builder<First>;

struct BasicBuilder;

struct Buildable {
    virtual ~Buildable() = default;
    virtual size_t GetSize() const = 0;
    virtual void Build(std::streambuf &buf) const = 0;
};

template<typename First, typename... Rest>
struct InstructionObject : public InstructionObject<Rest...> {
    InstructionObject() = default;

    InstructionObject(const First &first, Rest...rest)
        : InstructionObject<Rest...>(rest...),
          first(first)
    {
    }

    First first;

    virtual size_t GetSize() const override
        { return SizeImpl<First, Rest...>::GetSize(this); }

    virtual void Build(std::streambuf &buf) const override
        { Builder<First, Rest...>::Build(buf, this); }

    template<int Index>
    inline typename GetImpl<Index, First, Rest...>::type Get()
        { return GetImpl<Index, First, Rest...>::Get(this); }

    template<int Index, typename Value = typename GetImpl<Index, First, Rest...>::type>
    inline void Set(const Value &value)
        { GetImpl<Index, First, Rest...>::Set(this, value); }
};

template<typename First>
struct InstructionObject<First> : public Buildable {
    InstructionObject() = default;

    InstructionObject(const First &first)
        : first(first)
    {
    }

    virtual ~InstructionObject() = default;

    First first;

    virtual size_t GetSize() const override
        { return SizeImpl<First>::GetSize(this); }

    virtual void Build(std::streambuf &buf) const override
        { Builder<First>::Build(buf, this); }
  
    template<int Index>
    inline typename GetImpl<Index, First>::type Get()
        { return GetImpl<Index, First>::Get(this); }
  
    template<int Index>
    inline void Set(const First &value)
        { GetImpl<Index, First>::Set(this, value); }
};

template<int Index, typename First, typename... Rest>
struct GetImpl {
    using type = typename GetImpl<Index - 1, Rest...>::type;    
        
    static inline typename GetImpl<Index - 1, Rest...>::type
    Get(InstructionObject<First, Rest...> *obj)
        { return GetImpl<Index - 1, Rest...>::Get(obj); }

    template <typename Value = typename GetImpl<Index - 1, Rest...>::type>
    static inline void Set(InstructionObject<First, Rest...> *obj, const Value &value)
        { GetImpl<Index - 1, Rest...>::Set(obj, value); }
};

template<typename First, typename... Rest>
struct GetImpl<0, First, Rest...> {
    using type = First;    

    static inline First &Get(InstructionObject<First, Rest...> *obj)
        { return obj->first; }
  
    static inline void Set(InstructionObject<First, Rest...> *obj, const First &value)
        { obj->first = value; }
};

struct SizeHelper {
    static inline size_t GetSizeOfElem(const char *str)
        { return std::strlen(str); }

    template<typename T>
    static inline size_t GetSizeOfElem(const T &t)
        { return sizeof(t); }
};

template<typename First, typename... Rest>
struct SizeImpl {
    static inline size_t GetSize(const InstructionObject<First, Rest...> *obj)
        { return SizeHelper::GetSizeOfElem(obj->first) + SizeImpl<Rest...>::GetSize(obj); }
};

template<typename First>
struct SizeImpl<First> {
    static inline size_t GetSize(const InstructionObject<First> *obj)
        { return SizeHelper::GetSizeOfElem(obj->first); }
};

struct BasicBuilder {
    static inline void Build(std::streambuf &buf, const char *str)
    {
        size_t sz = std::strlen(str);
        // do not copy NUL byte
        buf.sputn(str, sz);
    }

    template <typename T>
    static inline void Build(std::streambuf &buf, std::vector<T> ts)
    {
        const size_t sz = sizeof(T) * ts.size();
        buf.sputn((char*)&ts[0], sz);
    }

    template <typename T>
    static inline void Build(std::streambuf &buf, T t)
    {
        const size_t sz = sizeof(T);
        buf.sputn((char*)&t, sz);
    }
};

template<typename First, typename... Rest>
struct Builder {
    static inline void Build(std::streambuf &buf, const InstructionObject<First, Rest...> *obj)
    {
        BasicBuilder::Build(buf, obj->first);
        Builder<Rest...>::Build(buf, obj);
    }
};

template<typename First>
struct Builder<First> {
    static inline void Build(std::streambuf &buf, const InstructionObject<First> *obj)
        { BasicBuilder::Build(buf, obj->first); }
};

#endif