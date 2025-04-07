#include <QtCore/QByteArray>
#include <QtCore/QString>

static void func(const char *str) { }

static QByteArray returnBa() { return "foo"; }

static bool boolFunc(const char *s) { return s != nullptr; }

#define TEXT "macrotext"

struct S {
    QString key;
    QByteArray keyAsUtf8() { return key.toUtf8(); }
};

void test()
{
    QByteArray ba = "this is a qbytearray";

    const char *c = ba;
    func(ba);

    func(returnBa());

    func(QByteArray("foo"));
    func(QByteArray(TEXT " ba"));
    func(QByteArrayLiteral("ba-literal"));
    func(QByteArrayLiteral(TEXT " ba-literal"));

    QString utf16 = QString::fromLatin1("string");
    func(std::exchange(utf16, {}).toLocal8Bit());

    if (boolFunc(returnBa())) {
    }

    if (__builtin_expect(!!(boolFunc(returnBa())), true)) {
    }
    if (Q_LIKELY(boolFunc(returnBa()))) {
    }

    S s{};
    func(s.key.toUtf8());
    func(s.keyAsUtf8());

    func("foo" + ba + "bar");
}
