#include <QtCore/QDateTime>

void test()
{
    QDateTime::currentDateTime(); // OK
    QDateTime::currentDateTimeUtc(); // OK
    QDateTime::currentDateTime().toTime_t(); // Warning
    QDateTime::currentDateTime().toUTC(); // Warning


}
