#include <QtCore/QStringList>

QStringList produceList()
{
    return {"testme"};
}

QStringList produceOtherContainer()
{
    return {"testme"};
}

QStringList testReturnAndAppend()
{
    return produceList() << "moreeeeeeee";
}

QStringList testReturnAndAppendContainer()
{
    return produceList() << produceOtherContainer() << "moreeeeeeee";
}

void testWithInitialArgument()
{
    QStringList myList = QStringList(produceList()) << "abc" << "xyz";
}
