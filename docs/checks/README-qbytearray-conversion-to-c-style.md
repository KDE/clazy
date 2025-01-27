# qbytearray-conversion-to-c-style

Warns about usage of QByteArray to `const char *` implicit conversion operator. This
sort of conversion is deemed dangerous given how `const char *` is a very common type.
This can be fixed by making the conversion explicit, e.g. by using `QByteArray::constData()`.

Note that this conversion operator can be disabled by defining the `QBYTEARRAY_NO_CAST_TO_ASCII`
macro.

#### Caveats
For QByteArrayLiteral this check only works for Qt6 where QByteArrayLiteral is a macro, but not
with Qt5 where QByteArrayLiteral is a lambda.

#### Example

    void func(const char *);
    QByteArray ba = "some text";
    func(ba); // Bad, implicit conversion to const char *
    func(ba.constData()); // Good, explicit conversion

    func(QByteArrayLiteral("some literal")); // Bad, implicit conversion plus QByteArray allocation
    func("some literal")); // Good, use the string literal directly

#### Fixits

This check supports a fixit to rewrite your code. See the README.md on how to enable it.
