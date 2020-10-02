#include <QtCore/QLatin1Char>
#include <QtCore/QChar>
#include <QtCore/QString>
#include <QtCore/QDir>

void receivingQChar(QChar s1) {}
void receivingQLatin1Char(QLatin1Char s1) {}

#define PREFIX '*'
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

    QChar char_with_macro = QLatin1Char(PREFIX); // should not be fixed

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

}
