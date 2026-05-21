#include <QtCore/QObject>

inline void connectSth()
{
    auto obj = new QObject();
    QObject::connect(obj, qOverload<QObject *>(&QObject::destroyed), [](){}); // Useless qOverload, but should be excluded due to filename
}
