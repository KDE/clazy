# qt-keywords

Warns when using Qt keywords such as `emit`, `slots`, `signals` or `foreach`.

This check is disabled by default and must be explicitly enabled, since using the above Qt keywords is fine unless you're using 3rdparty headers that also define them, in which case you'll want to use `Q_EMIT`, `Q_SLOTS`, `Q_SIGNALS` or `Q_FOREACH` instead.

It's also recommended you don't use the Qt keywords in public headers.

This check is mainly useful due to its *fixit* to automatically convert the keywords to their `Q_` variants. Once you've converted all usages, then simply enforce them via `CONFIG += no_keywords` (qmake) or `ADD_DEFINITIONS(-DQT_NO_KEYWORDS)` (CMake).
