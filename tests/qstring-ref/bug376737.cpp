#include <QtCore/QString>
#include <QtCore/QObject>
#include <QtCore/QVariant>

void test()
{
    QString line;
    QObject o;
    o.setProperty("size", line.mid(7).trimmed()); // OK
    QString attributes = line.mid(13).trimmed();  // OK

    QString comment;
    if (!comment.trimmed().isEmpty()) { // OK
    }
}
