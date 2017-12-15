#include <QtCore/QObject>

class MyObj; // OK

class MyObj : public QObject
{
public Q_SLOTS:
    void on_foo_bar();  // Warn

public:
    void on_foo2_bar2(); // OK

signals:
    void on_foo3_bar3(); // OK
};

class MyObj; // OK

void MyObj::on_foo_bar() // OK
{
}

void MyObj::on_foo2_bar2() // OK
{
}
