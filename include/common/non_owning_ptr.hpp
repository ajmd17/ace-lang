#ifndef NON_OWNING_PTR_HPP
#define NON_OWNING_PTR_HPP

template <typename T> class non_owning_ptr {
public:
    explicit non_owning_ptr() : m_ptr(nullptr) {}
    explicit non_owning_ptr(T *ptr) : m_ptr(ptr) {}
    explicit non_owning_ptr(const T *ptr) : m_ptr(ptr) {}
    non_owning_ptr(const non_owning_ptr &other)
        : m_ptr(other.m_ptr) {}
    ~non_owning_ptr() = default;

    inline T *get() { return m_ptr; }
    inline const T *get() const { return m_ptr; }

    inline non_owning_ptr &operator=(const non_owning_ptr &other)
        { m_ptr = other.m_ptr; return *this; }
    inline bool operator==(const non_owning_ptr &other) const
        { return m_ptr == other.m_ptr; }
    inline bool operator==(std::nullptr_t) const
        { return m_ptr == nullptr; }
    inline bool operator!=(const non_owning_ptr &other) const
        { return m_ptr != other.m_ptr; }
    inline bool operator!=(std::nullptr_t) const
        { return m_ptr != nullptr; }

    inline T *operator->() { return m_ptr; }
    inline const T *operator->() const { return m_ptr; }
    inline T &operator*() { return *m_ptr; }
    inline const T &operator*() const { return *m_ptr; }
    inline T &operator[](size_t i) { return m_ptr[i]; }
    inline const T &operator[](size_t i) const { return m_ptr[i]; }
private:
    T *m_ptr;
};

#endif
