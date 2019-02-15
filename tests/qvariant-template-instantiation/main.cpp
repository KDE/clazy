#include <QtCore/QVariant>
#include <QtCore/QUrl>


enum Foo {
    Foo1
};
Q_DECLARE_METATYPE(Foo)


void test()
{
    QVariant v;
    v.toUrl(); //ok
    v.toBool(); //ok
    v.value<QUrl>(); //warn
    v.value<bool>(); //warn
    v.value<float>(); //warn
    v.value<int>(); //warn


    v.value<Foo>(); //ok
}
