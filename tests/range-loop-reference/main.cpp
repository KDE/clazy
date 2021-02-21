#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QJsonArray>

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

void test_add_ref_fixits()
{
    QStringList strlist;
    for (const QString s : strlist) {} // should add &
    for (QString s : strlist) {} // should add const-&

    for (QString s : strlist) {  // shouldn't warn
        s = s.toLower();
    }

    for (QString s : strlist) {  // shouldn't warn
        s.clear();
    }
}

void test_json_array()
{
    QJsonArray array;
    const QJsonArray const_array;
    for (const auto a : array) {} // OK
    for (const QJsonValue a : const_array) {} // Warn
    for (const QJsonValue &a : const_array) {} // OK
}
