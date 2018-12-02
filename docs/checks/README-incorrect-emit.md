# incorrect-emit

For readability purposes you should always use emit (or Q_EMIT) when calling a signal.
Conversely, you should not use those macros when calling a non-signal.

clazy will warn if you forget to use emit (or Q_EMIT) or if you use them on a non-signal.

Additionally, it will warn when emitting a signal from a constructor, because there's likely nothing connected to the signal yet
(it could happen though, if the constructor itself, or something called by it, connects to that signal).
