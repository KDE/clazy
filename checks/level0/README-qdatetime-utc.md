# qdatetime-utc

Finds calls to `QDateTime::currentDateTime()` which should be replaced by
`QDateTime::currentDateTimeUTC()` in order to avoid expensive timezone code paths.

The two supported cases are:
- QDateTime::currentDateTime().to_timeT() -> QDateTime::currentDateTimeUtc().to_timeT()
- QDateTime::currentDateTime().toUTC() -> QDateTime::currentDateTimeUtc()
