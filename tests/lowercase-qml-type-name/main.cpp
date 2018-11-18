#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtQml/QQmlEngine>

struct Foo : public QObject {};

void test()
{
     qmlRegisterType<Foo>("App", 1, 0, "Foo"); // OK
     qmlRegisterType<Foo>("App", 1, 0, "foo"); // Warn
}
