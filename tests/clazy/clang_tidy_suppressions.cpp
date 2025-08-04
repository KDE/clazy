#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QDateTime>

void testClangTidySuppressionsSameLine()
{
    QString s = "linting for line disabled"; // NOLINT 
    if (s == "supressed nonexistent") {} // NOLINT(clazy-bla)
    if (s == "supressed all clazy checks") {} // NOLINT(clazy-*)
    if (s == "suppressed this check by name" && QDateTime::currentDateTime().toSecsSinceEpoch() < 0) {} // NOLINT(clazy-qstring-allocations)
}

void testClangTidySuppressionsNextLine()
{
    // NOLINTNEXTLINE
    QString s = "linting for line disabled";
    // NOLINTNEXTLINE(clazy-bla)
    if (s == "supressed nonexistent") {}
    // NOLINTNEXTLINE(clazy-*)
    if (s == "supressed all clazy checks") {}
    // NOLINTNEXTLINE(clazy-qstring-allocations)
    if (s == "suppressed this check by name" && QDateTime::currentDateTime().toSecsSinceEpoch() < 0) {}
}

void testRegionSupression()
{
    // NOLINTBEGIN
    QString s = "linting disabled";
    QString s1 = "linting disabled";
    QDateTime::currentDateTime().toSecsSinceEpoch();
    // NOLINTEND

    // NOLINTBEGIN(clazy-qdatetime-utc)
    QString warnme = "";
    const int suppressMe = QDateTime::currentDateTime().toSecsSinceEpoch();
    // NOLINTEND(clazy-qdatetime-utc)
}
