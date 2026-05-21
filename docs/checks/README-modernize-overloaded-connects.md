# modernize-overloaded-signals

Suggests to remove `QOverload`/`qOverload`/`static_cast` when a signal/slot is no longer overloaded.
Especially with removed overloads in Qt API, lots of places were not cleaned up.

In case a `static_cast` or `QOverload<>::of` is used, this check suggests utilizing the newer `qOverload` function.

Note: This check does not warn about overloaded signals existing. It merely suggests to use the most concise connect syntax. See the ["overloaded-signal"](./README-overloaded-signal.md) Clazy check.
Example of code where warnings/fixits are emitted:

```cpp
// Input
QObject::connect(obj, &MyClass::dummySignal, obj, QOverload<const QString &>::of(&MyClass::overloadedSlot));
QObject::connect(obj, &MyClass::dummySignal, obj, static_cast<void (MyClass::*)(const QString &)>(&MyClass::overloadedSlot));
// With fixit
QObject::connect(obj, &MyClass::dummySignal, obj, qOverload<const QString &>(&MyClass::overloadedSlot));
```

```cpp
// Input
menu.addAction("foo", obj, qOverload<>(&ReceiverClass::receiverSlot)); // QOverload is not needed
// With Fixit
menu.addAction("foo", obj, &ReceiverClass::receiverSlot);
```
