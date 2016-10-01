#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QDateTime>

// clazy:excludeall=foreach
// clazy:excludeall=qdatetime-utc comment with junk

void suppress_qstring_allocation()
{
    QString s = "foo"; // clazy:exclude=qstring-allocations
    if (s == "foo") {} // clazy:exclude=qstring-allocations
    if (s == "foo") {} // clazy:exclude=qstring-allocations comment with other junk
    if (s == "foo") {}
}

struct BigTrivial
{
    int a, b, c, d, e;
};

void suppress_foreach()
{
    QList<BigTrivial> list;
    foreach (BigTrivial b, list) { }
}

void qdatetimeutc()
{
    QDateTime::currentDateTime().toTime_t();
}
