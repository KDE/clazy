#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QDebug>

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
    QString escaped = "\\\\"; // Warning
    QString s13 = QString("foo"); // Warning
    QString s14 = QString(""); // Warning
    QString s15 = ""; // Warning
    s15.startsWith(QLatin1String("ff")); // OK
    s15.startsWith("ff"); // Warning, should be QLatin1String
    s15.contains("ff"); // Warning, should be QStringLiteral
    s15.indexOf("ff"); // Warning, should be QStringLiteral
}








struct A
{
    A() : member(QLatin1String("foo")) {} // Warning
    QString member;
};

struct A2
{
    A2() : member(QStringLiteral("foo")) // OK
         , member2(QString(QLatin1String("foo"))) {} // Warning
    void  test(const QString &calendarType = QLatin1String("gregorian") ); // Warning
    QString member;
    QString member2;
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

QStringList returns_stringlist(QString) { return {}; }

void test_foreach()
{
    foreach (const QString &s, returns_stringlist(QLatin1String("bar"))) // Warning
    {
    }
}

void testCharsets()
{
    QString s = "é";
    s += "é";
    s += "e";
    s.compare("é", "e");
    s.startsWith("é");
    s.startsWith("e");
    foreach (const QString &s, returns_stringlist(QLatin1String("éÈ"))) { }
    QString s11 = QString(QLatin1String("é"));

    if (s == QString::fromLatin1("foó"))
        return;
    if (s == QString::fromLatin1("foó %1").arg(1))
        return;
    s.contains(QString::fromLatin1("é"));
    s += QString::fromLatin1(
        "fóó" );
    QString s123 = QString::fromLatin1(some_char_pointer_indirection("fóó"));
    QString s9 = QLatin1String("stringéééé");
}

struct TestCharsets
{
    TestCharsets() : member(QStringLiteral("é"))
         , member2(QString(QLatin1String("é"))) {} // Warning
    void  test(const QString &calendarType = QLatin1String("éééé´e") );
    void test2();
    QString member;
    QString member2;
};











void moreShouldBeQLatin1String()
{
    QString s;
    bool b = (s == QString::fromLatin1("="));
    if (s.startsWith("Version=5")) {}
    if (s.startsWith(QLatin1String("Version=5"))) {} // OK
    QString result = QString::fromLatin1("Enum") + s;
}

void TestCharsets::test2()
{
    QString s = QString::fromLatin1("é"); // Can't fix this weird one
    QString s2 = QString::fromUtf8("é");
    s2 += QString::fromLatin1("é");
    s2 += QString::fromUtf8("é");
    s = "á";
    s += "á";
    QString("á");
    QStringList slist;
    slist << "á" << "a";
}





#include <QtCore/QRegExp>
void testBlacklistedQRegExp()
{
    QRegExp exp1("literal");
    QRegExp exp2(QLatin1String("literal"));
}


void charsetEdgeCase()
{
    test4(QLatin1String("ááá"));
}

void testQDebug()
{
    qDebug() << "test"; // OK
}

void testEmpty()
{
    QStringList list = QStringList() << "foo" << "";
}


void test_bug358732()
{
    QString("foo").sprintf("0x%02X", 0x1E); // Warn and use QSL
    QString("").sprintf("0x%02X", 0x1E); // Warn and use QSL
}


struct QTestData {};

template<typename T>
QTestData &operator<<(QTestData &data, const T &value);

void test_QTestData()
{
    QTestData t;
    t << QString::fromLatin1("foo");
}

void test_QStringList_streamOp()
{
    QStringList list;
    list << QStringLiteral("foo") << QLatin1String("foo") << QString::fromLatin1("foo") << "foo";
    list += QStringLiteral("foo");
    list += QLatin1String("foo");
    list += QString::fromLatin1("foo");
    list += "foo";
}

void testQLatin1String2Args()
{
    QString s2 = QLatin1String("foo", 3);
}

QString s = QString::fromUtf8("ö");
QString s2 = QString::fromUtf8("\xc3\xb6");
QString s3 = true ? "ö" : "\xc3\xb6";


// bug #391807
Q_GLOBAL_STATIC_WITH_ARGS(const QString, strUnit, (QLatin1String("unit"))) // OK, since QStringLiteral doesn't work inside Q_GLOBAL_STATIC_WITH_ARGS
