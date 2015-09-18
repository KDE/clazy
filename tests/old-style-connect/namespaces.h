#ifndef NAMESPACES_H_
#define NAMESPACES_H_

#include <QtCore/QObject>

namespace Foo {
    class Bar : public QObject
    {
        Q_OBJECT
    Q_SIGNALS:
        void signal1();

    };
}

class Mapper : public QObject
{
    Q_OBJECT
public:
    Mapper(Foo::Bar *obj, Foo::Bar *obj1) : QObject(nullptr)
    {
        connect(obj, SIGNAL(signal1()), obj1, SIGNAL(signal1()));
    }
};

#endif
