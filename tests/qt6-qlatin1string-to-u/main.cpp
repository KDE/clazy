#include <QtCore/QString>

int totototo = 112233;

void receivingQString(QString s1) {}
void receivingQLatin1String(QLatin1String s1) {}


#define PREFIX "foo"
void test()
{
    QString s1 = QLatin1String("str");
    QString s2 = QString(QLatin1String("s2"));
    s1 += QLatin1String("str");
    s1 = QLatin1String(true ? "foo" : "bar");
    s1.append(QLatin1String("appending"));

    receivingQString( QLatin1String("str"));
    receivingQLatin1String( QLatin1String("latin"));

    QLatin1String toto = QLatin1String("toto");

    const char* myChar = "foo";
    QString ww = QLatin1String(myChar); // should not be fixed
    QString string_with_macro = QLatin1String(PREFIX "bar"); // should not be fixed

}
