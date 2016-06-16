# dynamic-cast

Finds places where a dynamic cast is redundant.

#### Example

    Foo *a = ...;
    Foo *b = dynamic_cast<Foo*>(a);

or when dynamic casting to base class, which is uneeded.
