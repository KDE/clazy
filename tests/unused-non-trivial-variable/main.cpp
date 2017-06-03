#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QVector>
#include <QtCore/QByteArray>
#include <QtCore/QRect>






extern void external(QString);

QString test()
{
    QString s; // Warning
    QString s1, s2; // Warning for s2
    QString s3; // OK
    external(s1);

    return s3;
    return {};
}

struct MyRAII
{
    MyRAII();
    ~MyRAII();
};

void testRAII()
{
    MyRAII m; // OK
}

void testFor()
{
    QStringList l;
    for (QString s : l) // OK
        s;

    foreach (QString s,  l) // OK
        s;
}


void test4()
{
    QList<int> l; //Warn
    QVector<int> v; //Warn
    QByteArray b; //Warn
    QRect r; // Warn
}
