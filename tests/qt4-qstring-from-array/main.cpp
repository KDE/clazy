#include <QtCore/QString>
#include <QtCore/QByteArray>




QByteArray returns_byte_array() { return {}; }
void receivesQString(const QString &) {}







void test()
{
    QByteArray bytearray;
    QString s1("test");
    QString s2(bytearray);
    QString s3(bytearray + bytearray);
    QString s4 = QString("test");
    QString s5 = QString(bytearray);
    QString s6 = QString(bytearray + bytearray);
    QString s7 = QString(); // OK
    QString s8 = QString(QString()); // OK
    s1 = "test";
    s1 = "test"
          "bar";
    s1 = bytearray;

    if (s1 == "test") {}
    if (s1 == bytearray) {}
    if (s1 == bytearray + "test") {}

    s1 = bytearray + bytearray;
    s1 += bytearray;
    s1 += bytearray + bytearray;
    s1.append("foo");
    s1.prepend(bytearray);
    s1 = true ? "foo" : "bar";

    QString s9(returns_byte_array() + bytearray);
    s1.append(returns_byte_array());

    receivesQString("test");
}
