# fully-qualified-moc-types

Warns when a signal, slot or invokable declaration is not using *fully-qualified* type names, which will break *old-style* connects and interaction with QML.

Also warns if a `Q_PROPERTY` of type gadget is not fully-qualified (Enums and `QObject`s in `Q_PROPERTY` don't need
to be fully qualified).

Example:
```
namespace MyNameSpace {

    struct MyType { (...) };

    class MyObject : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(MyGadget myprop READ myprop); // Wrong, needs namespace
    Q_SIGNALS:
        void mySignal(MyType); // Wrong
        void mySignal(MyNameSpace::MyType); // OK
    };
}
```
Beware that fixing these type names might break user code if they are connecting to them via *old-style* connects, since the users might have worked around your bug and not included the namespace in their connect statement.

The `Q_PROPERTY` warning is only given when processing your *.moc files, it won't work if you for some reason compile an header file individually.
Meaning it won't warn when using QtCreator's inline clazy support. Only when doing a complete build with clazy (As in QMAKE_CXX=clazy / CXX=clazy).
