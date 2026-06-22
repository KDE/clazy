#include <QtCore/QObject>
#include <QtCore/QString>
QString produceString()
{
    return "";
}
void testVarInitialization()
{
    QStringList myList = QStringList() << "abc" << "xyz" << "123";
    QStringList myList2 = (QStringList() << "abc" << "xyz");
    QList<int> myList3 = QList<int>() << 1 << 2;
    QStringList myList4 = (QStringList() << "abc" << "xyz" << produceString());
}

void consumeList(QStringList args)
{
    Q_UNUSED(args)
}

void testFunctionArgs()
{
    consumeList(QStringList() << "comsume" << "meeee");
}
void testWithComment()
{
    QStringList myList = (QStringList() << "abc" // BLA
                                        << "xyz" // BLUB
    );
    QStringList myList2 = (QStringList() << "abc" /*data1*/ << "xyz" /*data2*/);
}

void testFromList()
{
    QStringList abc = QVector<QString>::fromList({"abc"}) << "xyz";
}
