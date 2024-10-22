#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QVector>
#include <QtCore/QByteArray>
#include <QtCore/QRect>
#include <QtCore/QJsonValue>
#include "other.h"




extern void external(QString);

QString test()
{
    QString s; // Warning
    [[maybe_unused]] QString s_unused; // OK - has attribute

    QString s1, s2; // Warning for s2
    [[maybe_unused]] QString s3_unused, s4_unused; // OK - both have attribute

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
    MyRAII m; // OK, not whitelisted
    [[maybe_unused]] MyRAII m_unused; // OK - has attribute
}

void testFor()
{
    QStringList l;
    for (QString s : l) // OK
        s;
    for ([[maybe_unused]] QString s : l) // OK - has attribute
        ;
    foreach (QString s, l) // OK
        s;
}


void test4()
{
    QList<int> l; //Warn
    [[maybe_unused]] QList<int> l_unused; // OK - has attribute

    QVector<int> v; //Warn
    [[maybe_unused]] QVector<int> v_unused; // OK - has attribute

    QByteArray b; //Warn
    [[maybe_unused]] QByteArray b_unused; // OK - has attribute

    QRect r; // Warn
    [[maybe_unused]] QRect r_unused; // OK - has attribute

    QJsonValue jsv; // Warn
    [[maybe_unused]] QJsonValue jsv_unused; // OK - has attribute

    FOO(QRect) r2; // OK
    r2.setX(0);
}

struct MyWhitelistedType
{
    MyWhitelistedType();
    ~MyWhitelistedType();
};

void testUserWhitelist()
{
    MyWhitelistedType m; // OK, whitelisted
    [[maybe_unused]] MyWhitelistedType m_unused; // OK - whitelisted and has attribute
}

// Additional test cases for [[maybe_unused]]
void testMixedScenarios()
{
    [[maybe_unused]] QList<int> list1; // OK - has attribute
    QList<int> list2; // Warning

    [[maybe_unused]] QString str1("test"); // OK - has attribute even if could be used
    QString str2; // Warning

    QStringList items;
    for ([[maybe_unused]] const QString &item : items) // OK - has attribute
    {
        // Do nothing with item
    }
}

void testNestedContainers()
{
    [[maybe_unused]] QList<QVector<int>> nested1; // OK - has attribute
    QList<QVector<int>> nested2; // Warning

    [[maybe_unused]] QVector<QString> vec1; // OK - has attribute
    QVector<QString> vec2; // Warning
}
