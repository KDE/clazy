# base-class-event

Warns when you `return false` inside your `QObject::event()` or `QObject::eventFilter()` reimplementation.
Instead you should probably call the base class method, so the event is correctly handled.
