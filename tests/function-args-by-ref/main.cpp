#include <QtCore/QDebug>
#include <QtGui/QColor>



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

extern void by_pointer(NonTrivial*);
extern void by_const_pointer(const NonTrivial*);
extern void by_ref(NonTrivial&);
extern void by_constRef(const NonTrivial&);

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


struct Ctors2
{
    Ctors2(NonTrivial n) : m(std::move(n)), i(0) {} // Ok
    NonTrivial m;
    int i;
};

struct Ctors3
{
    Ctors3(NonTrivial n) : m(n), i(0) {} // Warning
    NonTrivial m;
    int i;
};
