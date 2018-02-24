# dynamic-cast

Finds places where a dynamic cast is redundant or when dynamic casting to base class, which is unneeded.
Optionally it can also find places where a qobject_cast should be used instead.

#### Example

    Foo *a = ...;
    Foo *b = dynamic_cast<Foo*>(a);


If you prefer to use qobject_cast instead of dynamic_cast you can catch those cases with:
`export CLAZY_EXTRA_OPTIONS="unneeded-cast-qobject`
