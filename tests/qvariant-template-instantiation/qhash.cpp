#include <QtCore/QVariant>
#include <QtCore/QUrl>


enum Foo {
    Foo1
};
Q_DECLARE_METATYPE(Foo)


void test()
{
    QHash<QString, QVariant> properties;
    properties.insert("name", "John Doe");
    properties.insert("age", 42);
    properties.value("name").toString(); //Ok
    properties["age"].value<int>(); //Warn
    
    properties["name"].value<Foo>(); //Ok
}