#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QVector>
#include <QtCore/QByteArray>
#include <QtCore/QRect>
#include <QtCore/QMutex>
#include <QtCore/QScopedPointer>
#include "other.h"



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
    MyRAII m; // Warn, not blacklisted
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
    FOO(QRect) r2; // OK
    r2.setX(0);
}

void mutex()
{
    QMutex m;
    QMutexLocker ml(&m); // OK, is uninteresting
    QScopedPointer<QMutex> p(&m);  // OK, is uninteresting
}

struct MyBlacklistedType
{
    MyBlacklistedType();
    ~MyBlacklistedType();
};

void testUserWhitelist()
{
    MyBlacklistedType m; // OK, blacklisted
}
