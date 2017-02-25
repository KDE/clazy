#include <QtCore/QString>
#include <QtCore/QObject>
#include <QtCore/QVariant>

void test()
{
    QString line;
    QObject o;
    o.setProperty("size", line.mid(7).trimmed()); // OK
}
