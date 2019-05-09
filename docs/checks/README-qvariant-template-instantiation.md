# qvariant-template-instantiation

Detects when you're using `QVariant::value<Foo>()` instead of `QVariant::toFoo()`.

The former results in more code being generated in theory.

## False-Positives
Beware that this function doesn't work well when typedefs are involved, for example
it suggests that `v.toUlongLong()` should replace `v.value<quintptr>()`, which
isn't semantically correct.
