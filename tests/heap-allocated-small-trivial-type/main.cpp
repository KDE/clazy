#include <QtCore/QObject>
#include <QtCore/QString>

struct SmallTrivial
{
    int v;
};

struct BigTrivial
{
    int v[10];
};

struct NonTrivial
{
    int v;
    ~NonTrivial() {}
};

struct NonTrivial2
{
    int v;
    NonTrivial2() {}
    NonTrivial2(const NonTrivial2 &) {}
};

void test()
{
    auto a = new SmallTrivial(); // Warn
    auto b = new BigTrivial();
    auto c = new NonTrivial();
    auto d = new NonTrivial2();

    auto e = new(0) SmallTrivial;
    auto f = new SmallTrivial[100];
}

class MyClass
{
    void myMethod()
    {
        auto a = new SmallTrivial(); // OK
        m_smallTrivial = a;
        m_smallTrivial = new SmallTrivial(); // OK
    }

    SmallTrivial *m_smallTrivial = nullptr;
};

void receivesSmallTrivial(SmallTrivial *)
{
}


SmallTrivial* test2()
{
    return new SmallTrivial(); // OK
}

SmallTrivial* test3()
{
    auto a = new SmallTrivial(); // OK
    receivesSmallTrivial(a);
    return a;
}

SmallTrivial* test4()
{
    auto a = new SmallTrivial(); // OK
    return a;
}
