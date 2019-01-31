#include <QtCore/QDebug>
#include <QtGui/QColor>
#include <atomic>


struct Trivial {
    int a;
};

struct NonTrivial {
    NonTrivial() {}
    NonTrivial(const NonTrivial &) {}
    void constFunction() const {};
    void nonConstFunction() {};
    int a;
};

void by_pointer(NonTrivial*) {}
void by_const_pointer(const NonTrivial*) {}
void by_ref(NonTrivial&) {}
void by_constRef(const NonTrivial&) {}

int foo1(const Trivial nt) // Test #1: No warning
{
    return nt.a;
}

int foo2(const NonTrivial nt) // Test #2: Warning
{
    nt.constFunction();
    return nt.a;
}

int foo3(NonTrivial nt) // Test #3: Warning
{
    return nt.a;
}

int foo4(NonTrivial nt) // Test #4: Warning
{
    return nt.a;
}

int foo5(NonTrivial nt) // Test #5: No warning
{
    by_pointer(&nt);
    return nt.a;
}


int foo6(NonTrivial nt) // Test #6: No warning
{
    by_ref(nt);
    return nt.a;
}

int foo7(NonTrivial nt) // Test #7: No warning
{
    nt.nonConstFunction();
    return nt.a;
}

int foo8(NonTrivial nt) // Test #8: Warning
{
    nt.constFunction();
    return nt.a;
}

int foo9(NonTrivial nt) // Test #9: Warning
{
    by_constRef(nt);
    return nt.a;
}

int foo10(NonTrivial nt) // Test #10: Warning
{
    by_const_pointer(&nt);
    return nt.a;
}

int foo11(QColor) // Test #11: No warning
{
    return 1;
}

int foo12(NonTrivial nt) // Test #12: No warning
{
    nt = NonTrivial();
    return 1;
}

void func(QString text)
{
    QTextStream a(&text); // OK, by-pointer to a CTOR
}

struct StringConstPtr
{
    StringConstPtr(const QString *s) {}
};

void func2(QString text)
{
    StringConstPtr s(&text); // Warning, it's a const ptr
}

enum Option {
    FooOption,
    FooOption2
};


Q_DECLARE_FLAGS(Options, Option)
Q_DECLARE_OPERATORS_FOR_FLAGS(Options)

struct NoUserCtorsButNotTrivial
{
    NonTrivial t;
};

void testNoUserCtorsButNotTrivial(NoUserCtorsButNotTrivial)
{

}

struct TrivialWithDefaultCopyCtorAndDtor
{
    TrivialWithDefaultCopyCtorAndDtor(const TrivialWithDefaultCopyCtorAndDtor &) = default;
    ~TrivialWithDefaultCopyCtorAndDtor() = default;
    int m;
};

void testTrivialWithDefaultCopyCtorAndDtor(TrivialWithDefaultCopyCtorAndDtor)
{

}

struct DefaultButNotTrivialCopyable
{
    DefaultButNotTrivialCopyable(const DefaultButNotTrivialCopyable & ) = default;
    ~DefaultButNotTrivialCopyable() = default;
    NonTrivial t;
};

void testDefaultButNotTrivialCopyable(DefaultButNotTrivialCopyable)
{

}

void shouldBeByValue(const Trivial &nt)
{

}


void isOK(const NonTrivial &&)
{
}

void foo(const Trivial &t = Trivial());
void foo1(const Trivial & = Trivial());
void foo2(const Trivial &t);
void foo3(const Trivial&);

void foo4(const Trivial &t);
void foo4(const Trivial &t)
{
}

void foo5(const Trivial &t)
{
}

void foo6(const Trivial &t = {})
{
}


struct Base // Test that fixits fix both Base and Derived
{
    void foo(const Trivial &);
};

void Base::foo(const Trivial &)
{
}

struct Derived : public Base
{
    void foo(const Trivial &);
};

void Derived::foo(const Trivial &)
{
}

struct QDBusMessage
{
    void createErrorReply(QString) {}
};

struct DeletedCtor // bug #360112
{
    Q_DISABLE_COPY(DeletedCtor)
};

struct CopyCtor // bug #360112
{
    CopyCtor(const CopyCtor &) {}
};

struct Ctors
{
    Ctors (NonTrivial) {}
};

void trivialByConstRef(const int &t) {} // Warn
void trivialByRef(int &t) {} // OK

// #381812

class BaseWithVirtuals
{
public:
    virtual void virtualMethod1(const Trivial &) {}; // Warn
    virtual void virtualMethod2(const Trivial &) {}; // Warn
    void nonVirtualMethod(const Trivial &) {}; // Warn
};

class DerivedWithVirtuals : BaseWithVirtuals {
public:
    void virtualMethod1(const Trivial &) override {}; // OK
    void virtualMethod2(const Trivial &) {}; // OK
    void nonVirtualMethod(const Trivial &) {}; // Warn
};


// bug #403088
void func(const std::atomic<int> &a) {} // Ok, since it's not trivially-copyable

// generalization of bug #403088
struct DeletedCopyCtor {
    DeletedCopyCtor(const DeletedCopyCtor &) = delete;
    int v;
};

void func(const DeletedCopyCtor &a) {}
