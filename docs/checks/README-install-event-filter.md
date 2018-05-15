# install-event-filter

Warns on potential misuse of `QObject::installEventFilter()`.
To install an event filter you should call `monitoredObject->installEventFilter(this)`, but sometimes
you'll write `installEventFilter(filterObject)` by mistake, which compiles fine.

In rare cases you might actually want to install the event filter on `this`, in which case this is a false-positive.
