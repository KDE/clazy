#include <QtCore/QCoreApplication>
#include <QtCore/QObject>

static QObject *s_obj;
extern QObject *getObject();

class A : public QObject
{
public:
    void childEvent(QChildEvent *ev)
    {
        auto obj1 = qobject_cast<QCoreApplication*>(ev->child()); // Warning
        auto obj2 = qobject_cast<QCoreApplication*>(s_obj); // OK
        auto obj3 = qobject_cast<QCoreApplication*>(getObject()); // OK
    }
};

