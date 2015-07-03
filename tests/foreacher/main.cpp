// clang+++ test.cpp -I /usr/include/qt/ -fPIC -lQt5Core -c

#include <QtCore/QList>

void test_detachment()
{
    // Test #1: Detaching the foreach container
    QList<int> list;
    foreach (int i, list) {
        list.first();
    }
}

struct Trivial
{
    int a;
};
Q_DECLARE_TYPEINFO(Trivial, Q_PRIMITIVE_TYPE);

struct BigTrivial
{
    int a, b, c, d, e;
    void constFoo() const {}
    void nonConstFoo() {}
};

struct SmallNonTrivial
{
    int a;
    ~SmallNonTrivial() {}
};

extern void nop();
extern void nop2(BigTrivial &); // non-const-ref
extern void nop3(const BigTrivial &); // const-ref
void test_missing_ref()
{
    QList<Trivial> trivials;
    QList<BigTrivial> bigTrivials;
    QList<SmallNonTrivial> smallNonTrivials;

    // Test #2: No warning
    foreach (Trivial t, trivials) {
        nop();
    }

    // Test #3: Warning
    foreach (BigTrivial t, bigTrivials) {
        nop();
    }

    // Test #4: Warning
    foreach (SmallNonTrivial t, smallNonTrivials) {
        nop();
    }

    // Test #5: Warning
    foreach (const BigTrivial t, bigTrivials) {
        t.constFoo();
    }

    // Test #6: No warning
    foreach (BigTrivial t, bigTrivials) {
        t.nonConstFoo();
    }

    // Test #7: No warning
    foreach (BigTrivial t, bigTrivials) {
        t = BigTrivial();
        t++;
        --t;
    }

    // Test #8: No warning
    foreach (BigTrivial t, bigTrivials) {
        nop2(t);
    }

    // Test #9: No warning
    foreach (BigTrivial t, bigTrivials) {
        nop3(t);
    }
}
