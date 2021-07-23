# use-static-qregularexpression

Finds places such as

```cpp
    QString str = ...;
    if (str.contains(QRegularExpression(...))) { ... }

    QRegularExpression re(...);
    if (str.indexOf(re, 0)) { ... }
```

and suggests to use a `static` QRegularExpression object instead to avoid recreating the regular expressions.

Note that it only checks for functions using `QRegularExpression` from `QString` and `QStringList`.
