#include <QtCore/QObject>

struct NonQObject {
    void method() {}
};

void test()
{
    static auto f = &QObject::destroyed; // Warn
    auto f2 = &QObject::destroyed; // OK
    static auto f4 = &NonQObject::method; // OK
}
