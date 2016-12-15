// Ace IO library

#include <ace-sdk/ace-sdk.hpp>

#include <ace-vm/vm_state.hpp>
#include <ace-vm/exception.hpp>

#include <cstdio>

class File {
public:
    File(const char *filepath, const char *mode);
    File(const File &other);
    ~File();

    File &operator=(const File &other);

    inline bool operator==(const File &other) const
        { return m_file == other.m_file; }

    inline bool IsOpened() const { return m_file != nullptr; }

private:
    struct RefCounter {
        int m_count;
    };

    RefCounter *m_ref_counter;
    std::FILE *m_file;
};

File::File(const char *filepath, const char *mode)
    : m_ref_counter(new RefCounter), // create new ref counter
      m_file(std::fopen(filepath, mode)) // create new file
{
    printf("File constructor with ref count %d\n", m_ref_counter->m_count+1);
    m_ref_counter->m_count = 1;
}

File::File(const File &other)
    : m_ref_counter(other.m_ref_counter),
      m_file(other.m_file) // copy the pointers and increase ref count
{
    printf("File copy constructor with ref count %d\n", m_ref_counter->m_count+1);
    m_ref_counter->m_count++;
}

File::~File()
{
    printf("~File() with ref count %d\n", m_ref_counter->m_count-1);
    if ((--m_ref_counter->m_count) == 0) {
        delete m_ref_counter;
        // close file
        fclose(m_file);
    }
}

File &File::operator=(const File &other)
{
    if ((--m_ref_counter->m_count) == 0) {
        delete m_ref_counter;
        // close file
        fclose(m_file);
    }

    m_ref_counter = other.m_ref_counter;
    m_ref_counter->m_count++;
    m_file = other.m_file;

    return *this;
}

// io.open(filename, mode)
ACE_FUNCTION(io_open) {
    if (params.nargs != 2) {
        params.state->ThrowException(params.thread, Exception::InvalidArgsException(2, params.nargs));
        return;
    }

    StackValue *arg0 = params.args[0];
    ASSERT(arg0 != nullptr);

    StackValue *arg1 = params.args[1];
    ASSERT(arg1 != nullptr);

    Exception e = Exception(utf::Utf8String("open() expects arguments of type String and String"));

    utf::Utf8String *pathptr = nullptr;
    utf::Utf8String *modeptr = nullptr;

    if (arg0->GetType() == StackValue::ValueType::HEAP_POINTER) {
        if (arg0->GetValue().ptr == nullptr) {
            params.state->ThrowException(params.thread, Exception::NullReferenceException());
        } else if ((pathptr = arg0->GetValue().ptr->GetPointer<utf::Utf8String>()) == nullptr) {
            params.state->ThrowException(params.thread, e);
        } else {
            if (arg1->GetType() == StackValue::ValueType::HEAP_POINTER) {
                if (arg1->GetValue().ptr == nullptr) {
                    params.state->ThrowException(params.thread, Exception::NullReferenceException());
                } else if ((modeptr = arg1->GetValue().ptr->GetPointer<utf::Utf8String>()) == nullptr) {
                    params.state->ThrowException(params.thread, e);
                } else {
                    File file(pathptr->GetData(), modeptr->GetData());
                    if (!file.IsOpened()) {
                        // error opening file
                        params.state->ThrowException(params.thread,
                            Exception::FileOpenException(pathptr->GetData()));
                    } else {
                        // create an object to hold the file
                        // create heap value for the library
                        HeapValue *ptr = params.state->HeapAlloc(params.thread);
                        StackValue &res = params.thread->GetRegisters()[0];

                        ASSERT(ptr != nullptr);

                        // assign it to the library
                        ptr->Assign(file);

                        // assign register value to the allocated object
                        res.m_type = StackValue::HEAP_POINTER;
                        res.m_value.ptr = ptr;
                    }
                }
            } else {
                params.state->ThrowException(params.thread, e);
            }
        }
    } else {
        params.state->ThrowException(params.thread, e);
    }
}