# qt-keywords

Warns when using Qt keywords such as emit, slots, signals and foreach.

This check is disabled by default and must be explicitly enabled, as using the
above Qt keywords is fine unless you're using 3rdparty headers that also define them,
in which case you'll want to use Q_EMIT, Q_SLOTS, Q_SIGNALS and Q_FOREACH instead.

It's also recommended you don't use the Qt keywords in public headers.

Also consider using `CONFIG += no_keywords` (qmake) or `ADD_DEFINITIONS(-DQT_NO_KEYWORDS)` (CMake)
instead of using this check.
