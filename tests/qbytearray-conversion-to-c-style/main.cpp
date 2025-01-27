#include <QtCore/QByteArray>
#include <QtCore/QString>
static void func(const char *str) { }

static void otherFunc(const char *str) { }

#define TEXT "macrotext"

void test()
{
    QByteArray ba = "this is a qbytearray";

    const char *c = ba;
    func(ba);

    otherFunc(QByteArray("foo"));
    otherFunc(QByteArrayLiteral("ba-literal"));
    otherFunc(QByteArrayLiteral(TEXT " ba-literal"));

    QString utf16 = QString::fromLatin1("string");
    func(std::exchange(utf16, {}).toLocal8Bit());
}
