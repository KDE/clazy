#include <QtCore/QString>
#include <QtCore/QList>

// clazy:excludeall=foreach

void suppress_qstring_allocation()
{
    QString s = "foo"; // clazy:exclude=qstring-allocations
    if (s == "foo") {} // clazy:exclude=qstring-allocations
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
