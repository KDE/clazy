#include <QtCore/QObject>

class A : public QObject
{
    Q_OBJECT
public:
    int g;
    A()
        : g(foo()) // OK
    {
    }
    bool foo_bool() const
    {
        return true;
    }
    int foo() const
    {
        return 5;
    }
    void bar() const
    {
        foo(); // WARN
        if (foo()) // OK
        { }
    }
};

class B
{
public:
    A a;
    B()
        : g(a.foo()) // OK
    {
    }
    int g;
};

class C
{
public:
    A a;
};

int main(int argc, char *argv[])
{
    A a;
    B b;
    a.bar();
    auto lambda = [](A &obj) {
        obj.foo(); // WARN
    };
    lambda(a);

    C *c = new C();
    if (c->a.foo_bool()) { // OK
    }
    while (c->a.foo_bool()) { // OK
        A obj;
        obj.foo(); // WARN

        int x = 10;
        switch (c->a.foo()) { // OK
        case 1:
            A bar;
            bar.foo(); // WARN
            break;
        }

        for (int i = 0; i < (c->a.foo()); i++) {
            A bar;
            bar.foo(); // WARN
        }

        do {
            A bar;
            bar.foo(); // WARN
        } while (c->a.foo_bool()); // OK
    }
    return 0;
}
