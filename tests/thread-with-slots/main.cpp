#include <QtCore/QThread>
#include <QtCore/QMutexLocker>

void external(int foo);

class MyThread;
class MyThread : public QThread
{
public Q_SLOTS:
    void slot1() {} // OK
    void slot2();
    void slot4();
    void slot5();
    void slot6();
public:
    void slot3();
    int m_foo;
};

void MyThread::slot2() { m_foo = 1; }  // Warn

void  MyThread::slot5() {}
void  MyThread::slot6() { external(m_foo); } // Warn

QMutex s_m;
void MyThread::slot4() // OK
{
    QMutexLocker l(&s_m);
}

void slot3() {}

int main()
{
    MyThread *m;
    QThread *t;
    QObject *o;

    o->connect(o, &QObject::destroyed, m, &MyThread::slot1); // OK
    o->connect(o, &QObject::destroyed, m, &MyThread::slot3); // Warn
    o->connect(o, &QObject::destroyed, m, &QThread::requestInterruption); // OK
    o->connect(o, &QObject::destroyed, t, &QThread::requestInterruption); // OK

}
