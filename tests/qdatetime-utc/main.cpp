#include <QtCore/QDateTime>

void test()
{
    QDateTime::currentDateTime(); // OK
    QDateTime::currentDateTimeUtc(); // OK
    QDateTime::currentDateTime().toUTC(); // Warning
#if QT_VERSION_MAJOR == 5
    QDateTime::currentDateTime().toTime_t(); // Warning, but this method is removed in Qt6
#endif
    QDateTime::currentDateTime().toMSecsSinceEpoch(); // Warning
}
