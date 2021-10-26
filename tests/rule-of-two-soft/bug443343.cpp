struct Test
{
    Test() {}
    ~Test() { }

    Test(const Test &) {}
    Test& operator=(const Test&) = default;
};

void test()
{
    Test t;
    Test t2;
    t = t2; // OK, the developer explicitly says he wants the default copy-assign op
}

struct Test2
{
    Test2() {}
    ~Test2() { }

    Test2(const Test2 &) {}
};

void test2()
{
    Test2 t;
    Test2 t2;
    t = t2; // Warn
}
