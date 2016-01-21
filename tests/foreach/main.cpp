// clang+++ test.cpp -I /usr/include/qt/ -fPIC -lQt5Core -c
#include <vector>
#include <QtCore/QList>
#include <QtCore/QStringList>
#include <QtCore/QMap>
#include <QtGui/QRegion>
#include <QtCore/QVector>
#include <QtCore/QSequentialIterable>



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
extern void nop4(BigTrivial *); // pointer

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


    }

    // Test #8: No warning
    foreach (BigTrivial t, bigTrivials) {
        nop2(t);
    }

    // Test #9: Warning
    foreach (BigTrivial t, bigTrivials) {
        nop3(t);
    }

    // Test #9: No warning
    foreach (BigTrivial t, bigTrivials) {
        nop4(&t);
    }
}

void testSTLForeach()
{
    std::vector<int> v = {1, 2, 3, 4};
    foreach (int i, v) { // Warning
    }
}

void testQStringList()
{
    QStringList sl;
    sl << QChar('A') << QChar('B');
    foreach (const QString &s, sl) { // no warning
    }
}


void testQMultiMapDetach()
{
    QMultiMap<int,int> m;



    foreach (int i, m) {
        m.first();
    }
}

void testQRegionRects()
{
    QRegion r;
    foreach (const QRect &rect, r.rects()) {}
}

void varLengthArray()
{
    QVarLengthArray<int, 1> varray;
    foreach (auto i, varray) {}
}

void testQSequentialIterable()
{
    QVariant vlist;
    QSequentialIterable iterable = vlist.value<QSequentialIterable>();
    foreach (const QVariant &v, iterable) {}
}

void testMemberQList()
{
    struct { QStringList list; } data;
    foreach (const QString &s, data.list) {};
}
