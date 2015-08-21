#include <QtCore/QString>
#include <QtCore/QStringList>

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
    const char *cstring = "foo";
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
    s9 =  QLatin1String("string"); // Warning
    s9 =  QLatin1String(some_char_pointer_indirection("foo")); // OK
    QString s10 = true ? QLatin1String("string1") : QLatin1String("string2"); // Warning
    s10 = true ? QLatin1String("string1") :  QLatin1String("string2"); // Warning

    QString s11 = QString(QLatin1String("foo")); // Warning
    QStringList stringList;
    stringList << QString::fromLatin1("foo", 1); // OK
    QString s12 = QLatin1String(""); // OK, QString is optimized for the empty case
    QString s = QLatin1String(cstring + sizeof("foo")); // OK
    s = QString::fromLatin1("bar %1").arg(1); // Warning
    s = QString::fromLatin1(true ? "foo" : "foo2").arg(1); // Warning
    s += QString::fromLatin1(  // Warning: Multi-line, to test FIXIT
        "foo" );
    QString::fromLatin1(true ? cstring : "cstring"); // OK
    QString escaped = "\\\\"; // Warning, to test fixit
    QString s13 = QString("foo"); // Warning
    QString s14 = QString(""); // Warning
    QString s15 = ""; // Warning
}












struct A
{
    A() : member(QLatin1String("foo")) {} // Warning
    QString member;
};

struct A2
{
    A2() : member(QStringLiteral("foo")) {} // OK
    void  test(const QString &calendarType = QLatin1String("gregorian") ); // Warning
    QString member;
};

void test3()
{
    QString s;
    if (s == QString::fromLatin1("foo")) // Warning
        return;
    if (s == QString::fromLatin1("foo %1").arg(1)) // Warning
        return;
}

void test4(const QString &) {}

void test5()
{
    test4(QString::fromLatin1("foo")); // Warning
    QString s;
    s.contains(QString::fromLatin1("a")); // Warning
    s.compare(QStringLiteral("a"), QString::fromLatin1("a")); // Warning
}

#include <exception>
void exceptionTest() // Just to test a crash
{
    try {

    } catch (std::exception e) {

    }
}


