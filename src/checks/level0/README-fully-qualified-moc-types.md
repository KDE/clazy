# fully-qualified-moc-types

Warns when a signal declaration is not using fully-qualified type names, which will break old-style connects
and interacting with QML.

Example:

namespace MyNameSpace {

    struct MyType { (...) };

    class MyObject : public QObject
    {
        Q_OBJECT
    Q_SIGNALS:
        void mySignal(MyType); // Wrong
        void mySignal(MyNameSpace::MyType); // OK
    };
}

For Q_PROPERTIES it's not implemented yet.
Beware that fixing these signals might break user code if they are connecting to them via old style connects,
since the users might have worked around your bug and not included the namespace in their connect statement
