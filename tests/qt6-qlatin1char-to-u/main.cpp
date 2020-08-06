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

    int i = s.lastIndexOf(QLatin1Char('*'));

    const QString sc = "sc";
    QString s2 = QDir::cleanPath(sc + QLatin1Char('*'));

    s2.insert(1, QLatin1Char('*'));


    receivingQChar(QLatin1Char('/'));

    receivingQLatin1Char(QLatin1Char('/'));
    //error: no matching function for call to 'receivingQLatin1Char'
    //receivingQLatin1Char(u'/'); // remove check

    QLatin1Char toto = QLatin1Char('/');
    //error: no viable conversion from 'char16_t' to 'QLatin1Char'
    //QLatin1Char totoo = u'/'; // remove check

    QChar char_with_macro = QLatin1Char(PREFIX); // should not be fixed
}
