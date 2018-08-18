# unneeded-cast

Finds unneeded qobject_cast, static_cast and dynamic_casts.
Warns when you're casting to base or casting to the same type, which doesn't require
any explicit cast.

Also warns when you're using dynamic_cast for QObjects. qobject_cast is prefered.


#### Example

    Foo *a = ...;
    Foo *b = qobject_cast<Foo*>(a);


To shut the warnings about using qobject_cast over dynamic cast you can set:
`export CLAZY_EXTRA_OPTIONS="unneeded-cast-prefer-dynamic-cast-over-qobject"`

NOTE: This check has many false-positives. For example, you might cast to base
class to call a non-virtual method, or qobject_cast to itself to check if the
`QObject` destructor is currently being run.
