#include <QtCore/QLatin1Char>
#include <QtCore/QChar>
#include <QtCore/QString>
#include <QtCore/QDir>
//static const type var{QLatin1String(value)};
#define MY_CONSTANT(type, name, value)                \
static const type &name() {                           \
    static const type var{QLatin1String(QLatin1String(value))};      \
return var;                                           \
}
#define MY_STRING_CONSTANT(name, value) MY_CONSTANT(QString, name, value)

MY_STRING_CONSTANT(fooProperty, "foo")
MY_STRING_CONSTANT(barProperty, "bar") // Don't warn
MY_STRING_CONSTANT(bobProperty, QLatin1String(QLatin1String("bob")))

#define MY_CONSTANT_TEST(type, name, value)                \
static const type &name() {                           \
    static const type var{QString(value)};      \
return var;                                           \
}
#define MY_STRING_CONSTANT_TEST(name, value) MY_CONSTANT_TEST(QString, name, value)
MY_STRING_CONSTANT_TEST(bobProperty_test, QLatin1String("bob_test"))
MY_STRING_CONSTANT_TEST(bobProperty_test1, QLatin1String(QLatin1String("bob_test1")))
MY_STRING_CONSTANT_TEST(bobProperty_test2, QLatin1Char(false ? (true ? '*' : '/') : '/'))
#define MY_OTHER_CONSTANT(QString, name, value)                \
static const QString &name() {                           \
    static const QString var{QLatin1String(QLatin1String(value))};      \
return var;                                           \
}

MY_OTHER_CONSTANT(QString, other_test, QLatin1String("otherbob"))

void receivingQChar(QChar s1) {}
void receivingQLatin1Char(QLatin1Char s1) {}

void receivingQString(QString s1) {}
void receivingQLatin1String(QLatin1String s1) {}

bool return_bool(QString si) {return true;}

#define PREFIXX '*'
#define PREFIX "foo"
void test()
{
    QChar c1 = QLatin1Char('*');
    QChar c11 = u'*'; // remove the check

    QString s = "aaa";
    bool b = s.startsWith(QLatin1Char('/'));

    s += QLatin1Char('.');

    QString appPrefix = s + QLatin1Char('\\');
    appPrefix = s + QLatin1Char('\'');

    if (s.at(1) == QLatin1Char('*'))
        b = true;

    QChar quotes[] = { QLatin1Char('"'), QLatin1Char('"') };
    QChar c2 = QLatin1Char(true ? '*' : '/');

    bool myBool = true;
    QChar c3 = QLatin1Char(myBool ? (true ? '*' : '/') : '/');

    int i = s.lastIndexOf(QLatin1Char('*'));

    const QString sc = "sc";
    QString s2 = QDir::cleanPath(sc + QLatin1Char('*'));

    s2.insert(1, QLatin1Char('*'));


    receivingQChar(QLatin1Char('/'));

    receivingQLatin1Char(QLatin1Char('/'));

    QLatin1Char toto = QLatin1Char('/');

    QChar char_with_macro = QLatin1Char(PREFIXX); // should not be fixed

    QChar ccc = QLatin1Char(QLatin1Char(QLatin1Char('/')));
    c1 = QLatin1Char(true ? QLatin1Char(true ? '*' : '/') : QLatin1Char('*'));
    // nested QLatin1String should not be picked explicitly

    c1 = QLatin1Char(s.startsWith(QLatin1String("sd")) ? '/' : '*');// fix not supported (bool under CXXMemberCallExpr)

    // The outer QLatin1Char fix is not supported, but the inside ones are.
    c1 = QLatin1Char(s.startsWith(QLatin1Char('_')) ? '*' : '/');
    c1 = QLatin1Char(s.startsWith(QLatin1String(("aa"))) ?
                           QLatin1Char(true ? QLatin1Char('_') : QLatin1Char('_')) : QLatin1Char('_'));
    c1 = QLatin1Char(s.startsWith(QLatin1Char('/')) ?
                           QLatin1Char('_') : QLatin1Char(s.startsWith(QLatin1Char('_')) ? QLatin1Char('*') : QLatin1Char('/')));
    // Support fixit for the QLatin1Char("_") calls in the above cases

    QString s1 = QLatin1String("str");
    s2 = QString(QLatin1String("s2"));
    s1 += QLatin1String("str");
    s1 = QLatin1String(true ? "foo" : "bar");
    s1.append(QLatin1String("appending"));

    s1 = QLatin1String(myBool ? (true ? "foo" : "bar") : "bar");

    receivingQString( QLatin1String("str"));
    receivingQLatin1String( QLatin1String("latin"));

    QLatin1String totoo = QLatin1String("toto");

    QString sss = QLatin1String(QLatin1String(QLatin1String("str")));
    s1 = QLatin1String(true ? QLatin1String(true ? "foo" : "bar") : QLatin1String("bar"));
    // nested QLatin1String should not be picked explicitly

    const char* myChar = "foo";
    QString ww = QLatin1String(myChar); // fix not supported
    s1 = QLatin1String(myBool ? (true ? "foo" : myChar) : "bar"); // fix not supported (because of myChar)
    QString string_with_macro = QLatin1String(PREFIX "bar"); // fix not supported (macro present)
    s1 = QLatin1String(s.startsWith(QLatin1Char('/')) ? "foo" : "bar");// fix not supported (bool under CXXMemberCallExpr)

    // The outer QLatin1String fix is not supported, but the inside ones are.
    s1 = QLatin1String(s.startsWith(QLatin1String("fixme")) ? "foo" : "bar");//
    s1 = QLatin1String(s.startsWith(QLatin1Char('/')) ?
                           QLatin1String(true ? QLatin1String("dontfixme") : QLatin1String("dontfixme")) : QLatin1String("dontfixme"));
    s1 = QLatin1String(s.startsWith(QLatin1Char('/')) ?
                           QLatin1String("foo") : QLatin1String(s.startsWith(QLatin1String("fixme")) ? "foo" : "bar"));

    QString s2df = "dfg";
    s1 = QLatin1String(s.startsWith(QLatin1Char('/')) ? QLatin1String("dontfix1") : QLatin1String("dontfix2"));
    s1 = QLatin1String(return_bool(QLatin1String("fixme"))? QLatin1String("dontfix1") : QLatin1String("dontfix2"));
    s1 = QLatin1String(s2df.contains(QLatin1String("caught"))? QLatin1String("dontfix1") : QLatin1String("dontfix2"));

}

