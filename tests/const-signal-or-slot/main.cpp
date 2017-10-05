#include <QtCore/QObject>

class MyObject : public QObject
{
    Q_OBJECT
public:
    bool getter1() const;
    void nonGetter1() const;
    bool nonGetter2();
public Q_SLOTS:
    bool slot1() const { return true; } // Warn
    bool slot2() { return true; } // OK

    bool slot3() const; // Warn
    bool slot4(); // OK
    const int slot5(); // OK
    void slot6() const {} // OK
Q_SIGNALS:
    void signal1() const; // Warn
    void signal2(); // OK

};

bool MyObject::slot3() const // OK, already warned
{
    return true;
}

bool MyObject::slot4()
{
    return true;
}

void test()
{
    MyObject o;
    o.connect(&o, &MyObject::signal1, &MyObject::getter1); // Warn
    o.connect(&o, &MyObject::signal1, &MyObject::nonGetter1); // OK
    o.connect(&o, &MyObject::signal1, &MyObject::nonGetter2); // OK
}
