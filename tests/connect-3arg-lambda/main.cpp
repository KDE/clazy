#include <QtCore/QString>
#include <QtCore/QObject>


void test()
{
    QObject *o1;
    QObject *o2;
    QObject::connect(o1, &QObject::destroyed,
                    [=] { o2->deleteLater(); }); // Warn

    QObject::connect(o1, &QObject::destroyed, o2,
                    [=] { o2->deleteLater(); }); // OK

    QObject::connect(o1, &QObject::destroyed,
                    [=] { o1->deleteLater(); }); // OK

    QObject::connect(o1, &QObject::destroyed,
                    [=] { int a; a = 1; }); // OK
}

class MyObject : public QObject
{
public:
    void foo();

    void test()
    {
        MyObject *o2;
        connect(o2, &QObject::destroyed, [this] { // Warn
            foo();
        });

        connect(this, &QObject::destroyed, [this] { // OK
            foo();
        });

        connect(this, &QObject::destroyed, [o2] { // Warn
            o2->foo();
        });
    }
};
