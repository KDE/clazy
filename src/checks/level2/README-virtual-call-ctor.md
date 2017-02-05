# virtual-call-ctor

Finds places where you're calling pure virtual functions inside a constructor or destructor.
Compilers usually warn about this if there isn't any indirection, this check will catch cases like calling
a non-pure virtual that calls a pure virtual.


This check only looks for pure virtuals, ignoring non-pure, which in theory you shouldn't call,
but seems common practice.
