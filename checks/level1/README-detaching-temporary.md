# detaching-temporary

Finds places where you're calling non-const member functions on temporaries.
For example `getList().first()`, which would detach if the container is shared.

There can be some false-positives, for example `someHash.values().first()` because refcount is 1.
But `constFirst()` is a good default, so you should try to use it wherever you can, since it's not practical to inspect all code and figure out if the container is shared or not.
