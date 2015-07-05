#include <QtCore/QList>

QList<int> getList()
{
    return QList<int>();
}

void detach1()
{
    getList().first(); // Test #1: Warning
}

void detach2()
{
    getList().at(0); // Test #2: No warning
}
