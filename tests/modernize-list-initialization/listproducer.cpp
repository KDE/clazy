#include <QtCore/QStringList>

QStringList produceList()
{
    return {"testme"};
}

QStringList testReturnAndAppend()
{
    return produceList() << "moreeeeeeee";
}
