






struct A
{
    A()
    {
        indirection1();
    }

    ~A()
    {
        indirection1();
    }

    void indirection1()
    {
        some_virtual_method();
        some_non_virtual_method();
    }

    virtual void some_virtual_method() = 0;

    void some_non_virtual_method()
    {
    }
};
