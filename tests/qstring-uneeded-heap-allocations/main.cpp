#include <QtCore/QString>


const char * some_char_pointer_indirection(const char *)
{
    return nullptr;
}



const char * some_char_pointer()
{
    return nullptr;
}

void test()
{
    QString s1;
    s1.contains("string"); // Warning
    s1.contains(some_char_pointer()); // OK

    QString s2 = "string"; // Warning
    QString s3 = QStringLiteral("string"); // OK
    QString s4(some_char_pointer()); // OK

    s1 += "foo"; // Warning
    s1 += QStringLiteral("foo"); // OK
    s1 += QLatin1String("foo"); // OK

    QString s5 = QString::fromLatin1("BAR"); // Warning
    QString s6 = QString::fromUtf8("BAR"); // Warning
    QString s7 = QString::fromLatin1(some_char_pointer()); // OK
    QString s8 = QString::fromUtf8(some_char_pointer()); // OK
    QString s81 = QString::fromUtf8(some_char_pointer_indirection("foo")); // OK
    QString s123 = QString::fromLatin1(some_char_pointer_indirection("foo")); // OK

    QString s9 = QLatin1String("string"); // Warning
}
