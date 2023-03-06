#include <QtCore/QVariant>
#include <QtCore/QUrl>


enum Foo {
    Foo1
};
Q_DECLARE_METATYPE(Foo)


void test()
{
     QList<QVariant> list;
    list.append("John Doe");
    list.append(42);
    list.value(0).toString(); //Ok
    list[1].value<int>(); //Warn
    
    list[0].value<Foo>(); //Ok
}