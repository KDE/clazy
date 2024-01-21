# qdatetime-utc

Finds cases where less expensive QDateTime code paths can be used for getting the milliseconds/seconds since epoch.
This includes `QDateTime::currentDateTime().toUTC()` which should be replaced by `QDateTime::currentDateTimeUTC`()` to avoid expensive timezone code paths.
Getting the UTC datetime directly instead of resolving the current datetime is around 99% faster. In case the milliseconds/seconds since epoch are used,
the direct static accessors for those values halve the already improved time again.

## Examples:
```cpp
QDateTime::currentDateTime().toUTC()
QDateTime::currentDateTime().toTime_t() // Qt5 only
QDateTime::currentDateTime().toMSecsSinceEpoch()
QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
QDateTime::currentDateTime().toSecsSinceEpoch();
QDateTime::currentDateTimeUtc().toSecsSinceEpoch();
```
## Fixits

```
main.cpp:7:5: warning: Use QDateTime::currentDateTimeUtc() instead. It is significantly faster [-Wclazy-qdatetime-utc]
    QDateTime::currentDateTime().toUTC();
    ^        ~~~~~~~~~~~~~~~~~~~~~~~~~~~
             ::currentDateTimeUtc()

main.cpp:12:5: warning: Use QDateTime::currentMSecsSinceEpoch() instead. It is significantly faster [-Wclazy-qdatetime-utc]
    QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
    ^        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
             ::currentMSecsSinceEpoch()
main.cpp:14:5: warning: Use QDateTime::currentSecsSinceEpoch() instead. It is significantly faster [-Wclazy-qdatetime-utc]
    QDateTime::currentDateTimeUtc().toSecsSinceEpoch();
    ^        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
             ::currentSecsSinceEpoch()
```
