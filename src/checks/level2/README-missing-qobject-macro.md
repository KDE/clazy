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

This check can't be used with pre-compiled headers support.
This check doesn't have false positives, but it's not included in level <= 1 because the missing Q_OBJECT might be intentional.
