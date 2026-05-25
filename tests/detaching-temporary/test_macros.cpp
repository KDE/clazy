#include <QtTest/QTest>

QStringList produceTemporary()
{
    return QStringList("abc");
}

void test()
{
    QString value = produceTemporary()[0];
    QVERIFY(produceTemporary()[0] == "abc");
    QCOMPARE(produceTemporary()[0], "abc");
    QCOMPARE(produceTemporary() [ 0 ], "operator with spaces ");
    QCOMPARE(produceTemporary() [ 1 + 1 ], "expression within braces");
}
