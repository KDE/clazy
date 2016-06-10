# non-pod-global-static

Warns about non-POD [1] global statics.
CTORS from globals are run before main, on library load, slowing down startup.
This is more a problem for libraries, since usually the app won't use every feature the library provides,
so it's a waste of resources to initialize CTORs from unused features.

It's tolerated to have global statics in executables, however, clazy doesn't know if it's compiling
an executable or a library, so it's your job to run this check only on libraries. It doesn't harm, though,
to also remove global statics from executables, because they're usually evil.

The same goes for DTORs at library unload time. A good way to fix them is by using `Q_GLOBAL_STATIC`.


[1] The term "POD" is too strict. The correct term is "types with a trivial dtor and trivial ctor", and that's how this check is implemented.
