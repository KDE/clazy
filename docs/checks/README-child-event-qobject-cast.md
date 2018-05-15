# child-event-qobject-cast

Finds places where `qobject_cast<MyType>(event->child())` is being used inside `QObject::childEvent()` or equivalent (`QObject::event()` or `QObject::eventFilter()`).


`qobject_cast` can fail because the child might not be totally constructed yet.
