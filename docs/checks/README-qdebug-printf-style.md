# qdebug-printf-style

Finds `qDebug() << ...` statements that can be written in the printf-style `qDebug("...", ...)` form, and provides a fixit for them.

Streaming into `QDebug` emits a function call for every operand plus the construction and destruction of the `QDebug` object at every call site,
while the printf-style overload is a single call. This reduces code size, typically by a third to a half for the affected call sites.

Applies to `qDebug`, `qInfo`, `qWarning`, `qCritical` and the categorized `qCDebug`, `qCInfo`, `qCWarning`, `qCCritical`.

Example:

```cpp
qDebug() << "Loaded" << count << "items from" << fileName;
qCWarning(lcNetwork) << "Retrying, attempt" << attempt;
```

With fixit:

```cpp
qDebug("Loaded %d items from %ls", count, qUtf16Printable(fileName));
qCWarning(lcNetwork, "Retrying, attempt %d", attempt);
```

The fixit keeps the space that `QDebug` inserts between operands. Only statements whose operands are all of the following types are converted:

- string literals, which are inlined into the format string (with `%` escaped as `%%`)
- `const char *` (`%s`)
- integers (`%d`, `%u`, `%hd`, `%ld`, `%lld`, ...), `char` (`%c`), `float` and `double` (`%g`)
- `bool`, printed as `true`/`false`
- `QString` (`%ls` with `qUtf16Printable()`)

`size_t` and `ptrdiff_t` use `%zu` and `%td`. Other typedefs whose size differs between platforms, such as `qsizetype`, `int64_t`
or `quintptr`, are cast to `qint64`/`quint64` so the format string is correct everywhere, e.g. `qDebug("%lld", qint64(list.size()))`.

These statements are left alone:
- statements with any other operand, such as containers, enums, pointers, `QByteArray`, `QStringView`, bit-fields or
  objects converted through a user-defined conversion operator
- statements using `nospace()`, `noquote()` or stream manipulators such as `Qt::endl`
- statements whose `QDebug` result is used further, e.g. returned or bound to a reference
- statements containing comments or preprocessor directives, and logging through wrapper macros
- statements where the evaluation order of the operands could matter. Streamed operands are evaluated left to right, but function
  arguments in an unspecified order. So an operand that may change state, such as a call to a non-const function or `++i`, is
  only converted when all the other operands are constants. Calls to const member functions are assumed not to change state.

Note that the output changes for `QString` operands: `QDebug` prints them quoted and escaped (`"hello \"world\""`), while the
printf-style form prints them verbatim (`hello "world"`). All other supported types produce the same output.

This check is disabled by default and must be explicitly enabled, as the stream style is more convenient and supports more types.
