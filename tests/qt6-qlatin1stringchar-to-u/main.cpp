#include <QtCore/QLatin1Char>
#include <QtCore/QChar>
#include <QtCore/QString>
#include <QtCore/QDir>

void receivingQChar(QChar s1) {}
void receivingQLatin1Char(QLatin1Char s1) {}

void receivingQString(QString s1) {}
void receivingQLatin1String(QLatin1String s1) {}

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
                           QLatin1Char('_') : QLatin1Char(s.startsWith(QLatin1Char('_')) ? '*' : '/'));
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
                           QLatin1String(true ? QLatin1String("fixme") : QLatin1String("fixme")) : QLatin1String("fixme"));
    s1 = QLatin1String(s.startsWith(QLatin1Char('/')) ?
                           QLatin1Literal("fixme") : QLatin1String(s.startsWith(QLatin1String("fixme")) ? "foo" : "bar"));
    // Support fixit for the QLatin1Literal("fixme") calls in the above cases

}
