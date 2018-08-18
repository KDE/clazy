# connect-3arg-lambda

Warns when using the 3-arg `QObject::connect` that takes a lambda.
The recommendation is to use the 4-arg overload, which takes a context object
so that the lambda isn't executed when the context object is deleted.

It's very common to use lambdas to connect signals to slots with different number
of arguments. This can result in a crash if the signal is emitted after the receiver
is deleted.

Another reason for using a context-object is so you explicitly think about in
which thread you want the slot to run in. Note that with a context-object the
connection will be of type `Qt::AutoConnection` instead of `Qt::DirectConnection`,
which you can control if needed, via the 5th (optional) argument.


In order to reduce false-positives, it will only warn if the lambda body dereferences
at least one QObject (other than the sender).

It's very hard to not have any false-positives. If you find any you probably can just
pass the sender again, as 3rd parameter.

This will also warn if you pass a lambda to `QTimer::singleShot()` without
using the overload that takes a context object.
