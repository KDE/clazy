#include <QtCore/QObject>

class TestBugNew : public QObject
{
    void method();

    void otherMethod();

Q_SIGNALS:
    void someSignal();
};

void TestBugNew::method()
{
}

void TestBugNew::otherMethod()
{
    method(); // OK
}
