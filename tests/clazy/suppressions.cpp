#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QDateTime>

// clazy:excludeall=foreach
// clazy:excludeall=qdatetime-utc comment with junk

void suppress_qstring_allocation()
{   
    // NOLINTNEXTLINE
    QString s = "foo";
    if (s == "foo") {} // clazy:exclude=qstring-allocations
    if (s == "foo") {} // clazy:exclude=qstring-allocations comment with other junk
    if (s == "foo") {}
}

void suppress_qstring_allocation_scoped()
{
    {
        // clazy:exclude-scope=qstring-allocations
        QString s = "foo";
        // clazy:exclude-scope=warnme-qstring-allocations
        if (s == "foo") {
            QString scopeDisabled = "yxz";
        }
    }
    QString outOfDisabledScope = "abc";
    // clazy:exclude-next-line=qstring-allocations
    QString disabledPrevLine = "abc";
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
    QDateTime::currentDateTime().toSecsSinceEpoch();
}
