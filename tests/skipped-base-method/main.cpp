#include <QtCore/QString>

class Base {
public:
    virtual void method1() {}
    virtual void method2() {}
    virtual void method3() {}
};

class Derived : public Base {
public:
    virtual void method1() override {}
    void method3() override = 0;
};

class DerivedDerived : public Derived {
public:
    void test()
    {
        Base::method1(); // Warn
        Derived::method1(); // OK
        method1();

        Base::method2(); // OK
        Base::method3(); // OK
    }
};
