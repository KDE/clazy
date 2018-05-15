# lambda-unique-connection

Finds usages of `Qt::UniqueConnection` when the slot is a functor, lambda or non-member function.
That `connect()` overload does not support `Qt::UniqueConnection`.
