#include <QtCore/QObject>

class TestBugNew : public QObject
{
    Q_OBJECT

Q_SIGNALS:
    void someSignal();

protected:
    void protectedMethod();  // Needed to triggers the bug
};

// Needed to trigger the bug
void TestBugNew::someSignal()
{
}

// include subclass moc file which includes header of subclass with inline method using signal
class SubTestBugNew : public TestBugNew
{
    Q_OBJECT
public:
    void otherMethod();
};

void SubTestBugNew::otherMethod()
{
    emit someSignal(); // OK
}
