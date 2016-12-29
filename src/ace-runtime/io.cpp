// Ace IO library

#include <ace-sdk/ace-sdk.hpp>

#include <ace-vm/VMState.hpp>
#include <ace-vm/Exception.hpp>

#include <cstdio>

namespace ace {
namespace io {

class File {
public:
    explicit File(std::FILE *file, bool should_close);
    explicit File(const char *filepath, const char *mode, bool should_close);
    File(const File &other);
    ~File();

    File &operator=(const File &other);

    inline bool operator==(const File &other) const
        { return m_ref_counter->m_file == other.m_ref_counter->m_file; }
    inline bool IsOpened() const
        { return m_ref_counter->m_file != nullptr; }

    void Write(const utf::Utf8String &str);
    void Flush();
    void Close();

private:
    struct RefCounter {
        int m_count;
        std::FILE *m_file;
        bool m_should_close;

        RefCounter(int count, std::FILE *file, bool should_close)
            : m_count(count), m_file(file), m_should_close(should_close) {}
        RefCounter(const RefCounter &other) = delete;
        ~RefCounter() { if (m_file && m_should_close) std::fclose(m_file); }
    };

    RefCounter *m_ref_counter;
};

File::File(std::FILE *file, bool should_close)
    : m_ref_counter(new RefCounter(1, file, should_close)) // create new ref counter
{

}

File::File(const char *filepath, const char *mode, bool should_close)
    : m_ref_counter(new RefCounter(1, std::fopen(filepath, mode), should_close)) // create new ref counter
{
}

File::File(const File &other)
    : m_ref_counter(other.m_ref_counter) // copy the pointers and increase ref count
{
    m_ref_counter->m_count++;
}

File::~File()
{
    if (!(--m_ref_counter->m_count)) {
        delete m_ref_counter;
    }
}

File &File::operator=(const File &other)
{
    if (!(--m_ref_counter->m_count)) {
        delete m_ref_counter;
    }

    m_ref_counter = other.m_ref_counter;
    m_ref_counter->m_count++;

    return *this;
}

void File::Write(const utf::Utf8String &str)
{
    ASSERT(m_ref_counter != nullptr);
    ASSERT(m_ref_counter->m_file != nullptr);
    std::fputs(str.GetData(), m_ref_counter->m_file);
}

void File::Flush()
{
    ASSERT(m_ref_counter != nullptr);
    ASSERT(m_ref_counter->m_file != nullptr);
    std::fflush(m_ref_counter->m_file);
}

void File::Close()
{
    ASSERT(m_ref_counter != nullptr);
    ASSERT(m_ref_counter->m_file != nullptr);
    if (m_ref_counter->m_should_close) {
        std::fclose(m_ref_counter->m_file);
    }
    // set file to null
    m_ref_counter->m_file = nullptr;
}

} // namespace io
} // namespace ace

// io.open(filename: String, mode: String)
ACE_FUNCTION(io_open) {
    ACE_CHECK_ARGS(==, 2);

    using namespace ace;

    vm::Value *arg0 = params.args[0];
    vm::Value *arg1 = params.args[1];
    ASSERT(arg0 != nullptr);
    ASSERT(arg1 != nullptr);

    vm::Exception e = vm::Exception(utf::Utf8String("open() expects arguments of type String and String"));

    utf::Utf8String *path_ptr = nullptr;
    utf::Utf8String *mode_ptr = nullptr;

    if (arg0->GetType() == vm::Value::ValueType::HEAP_POINTER) {
        if (arg0->GetValue().ptr == nullptr) {
            params.state->ThrowException(params.thread, vm::Exception::NullReferenceException());
        } else if ((path_ptr = arg0->GetValue().ptr->GetPointer<utf::Utf8String>()) == nullptr) {
            params.state->ThrowException(params.thread, e);
        } else {
            if (arg1->GetType() == vm::Value::ValueType::HEAP_POINTER) {
                if (arg1->GetValue().ptr == nullptr) {
                    params.state->ThrowException(params.thread, vm::Exception::NullReferenceException());
                } else if ((mode_ptr = arg1->GetValue().ptr->GetPointer<utf::Utf8String>()) == nullptr) {
                    params.state->ThrowException(params.thread, e);
                } else {
                    io::File file(path_ptr->GetData(), mode_ptr->GetData(), true);
                    if (!file.IsOpened()) {
                        // error opening file
                        params.state->ThrowException(params.thread,
                            vm::Exception::FileOpenException(path_ptr->GetData()));
                    } else {
                        // create an object to hold the file
                        // create heap value for the library
                        vm::HeapValue *ptr = params.state->HeapAlloc(params.thread);
                        ASSERT(ptr != nullptr);

                        // assign it to the library
                        ptr->Assign(file);

                        vm::Value res;
                        res.m_type = vm::Value::HEAP_POINTER;
                        res.m_value.ptr = ptr;

                        ACE_RETURN(res);
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

// io_write(file: File, obj: Any...)
ACE_FUNCTION(io_write) {
    ACE_CHECK_ARGS(>=, 2);

    // first, read the file object (first argument)
    ace::vm::Value *arg0 = params.args[0];
    ASSERT(arg0 != nullptr);

    ace::vm::Exception e = ace::vm::Exception(utf::Utf8String("write() expects arguments of type File and Any..."));

    ace::io::File *file_ptr = nullptr;
    if (arg0->GetType() == ace::vm::Value::ValueType::HEAP_POINTER) {
        if (arg0->GetValue().ptr == nullptr) {
            params.state->ThrowException(params.thread, ace::vm::Exception::NullReferenceException());
        } else if ((file_ptr = arg0->GetValue().ptr->GetPointer<ace::io::File>()) == nullptr) {
            params.state->ThrowException(params.thread, e);
        } else {
            // found file object
            if (!file_ptr->IsOpened()) {
                params.state->ThrowException(params.thread, ace::vm::Exception::UnopenedFileWriteException());
            } else {
                // convert each arg to string
                for (int i = 1; i < params.nargs; i++) {
                    ASSERT(params.args[i] != nullptr);
                    // write to file
                    file_ptr->Write(params.args[i]->ToString());
                }

                // after writing, flush file buffer
                file_ptr->Flush();
            }
        }
    } else {
        params.state->ThrowException(params.thread, e);
    }
}

// io_close(file: File)
ACE_FUNCTION(io_close) {
    ACE_CHECK_ARGS(==, 1);

    // first, read the file object (first argument)
    ace::vm::Value *arg0 = params.args[0];
    ASSERT(arg0 != nullptr);

    ace::vm::Exception e = ace::vm::Exception(utf::Utf8String("close() expects argument of type File"));

    ace::io::File *file_ptr = nullptr;
    if (arg0->GetType() == ace::vm::Value::ValueType::HEAP_POINTER) {
        if (arg0->GetValue().ptr == nullptr) {
            params.state->ThrowException(params.thread, ace::vm::Exception::NullReferenceException());
        } else if ((file_ptr = arg0->GetValue().ptr->GetPointer<ace::io::File>()) == nullptr) {
            params.state->ThrowException(params.thread, e);
        } else {
            // found file object
            if (!file_ptr->IsOpened()) {
                params.state->ThrowException(params.thread, ace::vm::Exception::UnopenedFileCloseException());
            } else {
                // close the file
                file_ptr->Close();
            }
        }
    } else {
        params.state->ThrowException(params.thread, e);
    }
}

// io_get_stdout()
ACE_FUNCTION(io_get_stdout) {
    ACE_CHECK_ARGS(==, 0);

    // create an object to hold pointer to stdout
    // create heap value for the library
    ace::vm::HeapValue *ptr = params.state->HeapAlloc(params.thread);
    ASSERT(ptr != nullptr);

    ace::io::File file(stdout, false);

    // assign it to the library
    ptr->Assign(file);

    ace::vm::Value res;
    res.m_type = ace::vm::Value::HEAP_POINTER;
    res.m_value.ptr = ptr;

    ACE_RETURN(res);
}