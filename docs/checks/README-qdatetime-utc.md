# qdatetime-utc

Finds calls to `QDateTime::currentDateTime()` which should be replaced by
`QDateTime::currentDateTimeUTC()` in order to avoid expensive timezone code paths.

The two supported cases are:
- QDateTime::currentDateTime().toTime_t() -> QDateTime::currentDateTimeUtc().toTime_t()
- QDateTime::currentDateTime().toUTC() -> QDateTime::currentDateTimeUtc()
