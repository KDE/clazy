#include <QtCore/QString>

void test() // The QChar conversion is ambiguous in Qt6 resulting in a compilation error
{
    int a;
    QLatin1String("%1").arg(1); // Warn
    QLatin1String("%1").arg(a); // Warn
    QLatin1String("%1").arg(QChar(1)); // OK
    QLatin1String("%1").arg('a'); // OK
}
