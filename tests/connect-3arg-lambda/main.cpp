#include <QtCore/QString>
#include <QtWidgets/QMenu>
#include <QtCore/QTimer>
void another_global();
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

        connect(m_member, &QObject::destroyed, [this] { // Warn (this is dereferenced)
            m_member->deleteLater();
        });

        connect(m_member, &QObject::destroyed, [this] { // Warn
                test();
            });

        connect(m_member, &QObject::destroyed, [this] { // OK
                ::test();
            });

        connect(m_member, &QObject::destroyed, [] { // OK
                another_global();
            });

        MyObject *other;
        connect(other->m_member, &QObject::destroyed, [other] { // Warn (other might be deleted before)
            other->m_member->deleteLater();
        });

        connect(other->m_member, &QObject::destroyed, [other] { // Warn
            other->m_member2->deleteLater();
        });

        MyObject *other2;
        connect(other->m_member, &QObject::destroyed, [other, other2] { // Warn
            other2->m_member->deleteLater();
        });

    }

    QObject *m_member;
    QObject *m_member2;
};

void testTimer()
{
    QObject *o;
    QTimer::singleShot(0, [] {}); // Warn
    QTimer::singleShot(0, o, [] {}); // OK

    QTimer::singleShot(0, Qt::CoarseTimer, [] {}); // Warn
    QTimer::singleShot(0, Qt::CoarseTimer, o, [] {}); // OK
}

void testQMenu()
{
    MyObject o;
    QMenu menu;
    menu.addAction("foo", &o, &MyObject::test); // OK
    menu.addAction("foo", &o, []{}); // OK
    menu.addAction("foo", []{}); // Warn
}
