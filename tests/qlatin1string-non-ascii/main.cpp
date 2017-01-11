#include <QtCore/QString>

void test()
{
    QLatin1String s1("é"); // Warn
    QLatin1String s2("e"); // OK
    QLatin1String s3(""); // OK
    QLatin1String s4(s1); // OK
}
