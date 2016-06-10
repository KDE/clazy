# variant-template-instantiation

Detects when you're using `QVariant::value<Foo>()` instead of `QVariant::toFoo()`.

The former results in more code being generated.
