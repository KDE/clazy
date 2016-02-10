#include <QtCore/QString>

void test()
{
    QString s;
    s.toLower().startsWith("bar"); // Warning
    s.toLower().endsWith("bar"); // Warning
    s.toUpper().contains("bar"); // Warning
    s.toUpper().compare("bar"); // Warning
    s.compare("bar", Qt::CaseInsensitive); // OK
    s.contains("bar", Qt::CaseInsensitive); // OK
}
