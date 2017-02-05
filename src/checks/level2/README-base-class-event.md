# base-class-event

Warns when you `return false` inside your `QEvent::event()` or `QEvent::eventFilter()` re-implementation.
Instead you should probably call the base class method, so the event is correctly handled.
