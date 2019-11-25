#include <QtCore/QObject>
#include <QtCore/QString>

class MyQObject : public QObject
{
    Q_OBJECT

signals:
    void ok(); // OK
    void foo(); // Warn
    void foo(int); // Warn
    void bar(const QString &); // OK
};

class MyObject2 : public MyQObject
{
Q_OBJECT

signals:
    void bar(int); // Warn
};

void test()
{
}
