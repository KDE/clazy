#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <chrono>

using namespace std::chrono_literals;

void test()
{
    QTimer timer;

    timer.setInterval(1000); // Warn
    timer.setInterval(1200); // Warn
    timer.setInterval(120000); // Warn
    timer.setInterval(18000000); // Warn
    timer.setInterval(60*1000); // Warn
    timer.setInterval(15*60*1000); // Warn
    timer.setInterval(1000+1000); // Not really worth it
    timer.setInterval(1s); // OK
    timer.setInterval(1200ms); // OK
    timer.setInterval(0); // OK

    timer.start(15*1000); // Warn
    timer.start(15s); // OK
    timer.start(); // OK

    timer.singleShot(1000, [] {}); // Warn
    QTimer::singleShot(1000, [] {}); // Warn
}
