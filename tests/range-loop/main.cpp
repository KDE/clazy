#include <QtCore/QList>
#include <vector>
#include <QtCore/QMap>
#include <QtCore/QSequentialIterable>
using namespace std;
QList<int> getQtList()
{
    return {}; // dummy, not important
}

const QList<int> getConstQtList()
{
    return {}; // dummy, not important
}

const QList<int> & getConstRefQtList()
{
    static QList<int> foo;
    return foo;
}

void testQtContainer()
{
    QList<int> qt_container;
    for (int i : qt_container) { // Warning
    }

    const QList<int> const_qt_container;
    for (int i : const_qt_container) { // OK
    }

    for (int i : getQtList()) { // Warning
    }

    for (int i : qt_container) { }  // Warning
    for (const int &i : qt_container) { } // Warning
    for (int &i : qt_container) { } // OK




    for (int i : getConstQtList()) { // OK
    }

    for (int i : getConstRefQtList()) { // OK
    }

    vector<int> stl_container;
    for (int i : stl_container) { // OK
    }
}

class A {
public:
    void foo()
    {
        for (int a : m_stlVec) {} // OK
    }

    std::vector<int> m_stlVec;
};

void testQMultiMapDetach()
{
    QMultiMap<int,int> m;
    for (int i : m) {
    }
}

struct Trivial
{
    int a;
};

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
extern void nop4(BigTrivial *); // pointer

void test_missing_ref()
{
    const QList<Trivial> trivials;
    const QList<BigTrivial> bigTrivials;
    const QList<SmallNonTrivial> smallNonTrivials;

    // Test #2: No warning
    for (Trivial t : trivials) {
        nop();
    }

    // Test #3: No warning
    for (BigTrivial t : bigTrivials) {
        nop();
    }

    // Test #4: Warning
    for (SmallNonTrivial t : smallNonTrivials) {
        nop();
    }

    // Test #5: No Warning
    for (const BigTrivial t : bigTrivials) {
        t.constFoo();
    }

    // Test #6: No warning
    for (BigTrivial t : bigTrivials) {
        t.nonConstFoo();
    }

    // Test #7: No warning
    for (BigTrivial t : bigTrivials) {
        t = BigTrivial();
    }

    // Test #8: No warning
    for (BigTrivial t : bigTrivials) {
        nop2(t);
    }

    // Test #9: No Warning
    for (BigTrivial t : bigTrivials) {
        nop3(t);
    }

    // Test #9: No warning
    for (BigTrivial t : bigTrivials) {
        nop4(&t);
    }

    // Test #10: No warning (bug #362587)
    QSequentialIterable si = QVariant().value<QSequentialIterable>();
    for (const auto &s : si) {}
}
