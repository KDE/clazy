#include <QtCore/QVector>
#include <QtCore/QString>

struct A { A() {} };

extern void receivesByRef(QVector<int> &);
extern void receivesByRef2(const QString &, QVector<int> &);
extern void receivesByPtr(QVector<int> *);

void test()
{
    QVector<int> v1; // OK
    for (int i = 0; i < 10; i++) {
        A a; // OK
        QVector<int> v2; // Warning
        v2.append(i);
        QVector<int>().append(i); // OK (bogus but that's not what we're after)
    }
}

void test1()
{
    QVector<int> v;
    while (true) {
        QVector<int> v1(v); // OK
    }

    while (true) {
        QVector<int> v1; // OK
        receivesByRef(v1);
    }

    while (true) {
        QVector<int> v1; // OK
        receivesByRef2(QString(), v1);
    }

    while (true) {
        QVector<int> v1; // OK
        receivesByPtr(&v1);
    }
}
