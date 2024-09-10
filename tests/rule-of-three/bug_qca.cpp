template<typename T> class MemoryRegion
{
public:
    MemoryRegion<T> &operator=(const MemoryRegion<T> &)
    {
        return (*this);
    }

    ~MemoryRegion()
    {
    }

    MemoryRegion()
    {
    }
    MemoryRegion(const MemoryRegion<T> &)
    {
    }
};

