# incorrect-emit

For readability purposes you should always use emit (or Q_EMIT) when calling a signal.
Conversely, you should not use those macros when calling a non-signal.

clazy will warn if you forget to use emit (or Q_EMIT) or if you use them on a non-signal.
