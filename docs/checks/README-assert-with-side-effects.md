# assert-with-side-effects

Tries to find `Q_ASSERT`s with side-effects. Asserts are compiled-out in release mode so you shouldn't put any important code inside them.

#### Example
```
    // The connect statement wouldn't run in release mode
    Q_ASSERT(connect(buttonm, &QPushButton::clicked, this, &MainWindow::handleClick));
```

#### Pitfalls

This check is experimental and will have many false positives and might be buggy. Patches accepted!
