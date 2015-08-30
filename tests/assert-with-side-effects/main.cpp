#include <QtCore/qglobal.h>
#include <QtCore/QVector>
#include <QtCore/QString>








struct SomeStruct
{
    bool nonConstMember() { return false; }
    bool constMember() const { return false; }
    bool m;
};


bool global_func() { return false; }

void test()
{
    SomeStruct s;
    Q_ASSERT(s.nonConstMember()); // Warning, but ok with normal agressiveness
    Q_ASSERT(global_func()); // Warning, but ok with normal agressiveness

    int i;
    Q_ASSERT(i = 0); // Warning
}

class MyVector : QVector<int>
{
public:
    MyVector()
    {
        Q_ASSERT(!isEmpty()); // OK

        int a, b;
        Q_ASSERT(a <= b); // OK

        SomeStruct *s;
        Q_ASSERT(s->m); // OK
        Q_ASSERT(s->constMember()); // OK

        QString format;
        int i = 0;
        Q_ASSERT(format.at(i) == QLatin1Char('\'')); // OK
        Q_ASSERT(++i); // Warning
    }
};
