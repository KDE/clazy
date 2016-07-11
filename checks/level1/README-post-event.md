# post-event

Finds places where an event is not correctly passed to `QCoreApplication::postEvent()`.

`QCoreApplication::postEvent()` expects an heap allocated event, not a stack allocated one.
`QCoreApplication::sendEvent()` correctness is not checked due to false-positives.
