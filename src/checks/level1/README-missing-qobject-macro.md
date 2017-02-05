# missing-qobject-macro

Finds `QObject` derived classes that don't have a Q_OBJECT macro.

#### Reasons to use Q_OBJECT
- Signals and slots
- `QObject::inherits`
- `qobject_cast`
- `metaObject()->className()`
- Use your custom widget as a selector in Qt stylesheets

#### Reasons not to use Q_OBJECT
- Templated QObjects
- Compilation time

Requires clang >= 3.7
