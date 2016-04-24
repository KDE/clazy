#include <QtCore/QString>
#include <QtCore/QList>

// clazy:skip

void suppress_qstring_allocation()
{
    QString s = "foo";
    if (s == "foo") {}
}
