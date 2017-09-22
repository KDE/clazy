#include <QtCore/QObject>

class MyObject :public QObject
{
    Q_OBJECT
public:
    MyObject();
    void non_signal();
signals:
    void signal1();
    void signal2();
    void signal3();
    void signal4();
    void signal_with_params(int);
};

class MyObject_derived : public MyObject
{
    Q_OBJECT
public:
    MyObject_derived();
    void func();
signals:
    void signal1(); // Warn
};

class MyObject_derived_derived : public MyObject_derived
{
    Q_OBJECT
public:
    void func(); // OK
    void signal1() {} // Warn
    void signal4(); // Warn
signals:
    void signal2(); // Warn
    void non_signal(); // Warn
    void signal_with_params(int foo); // Warn
    void signal_with_params(char); // OK
};

void MyObject_derived_derived::non_signal() // OK. Already warned in declaration
{
}
