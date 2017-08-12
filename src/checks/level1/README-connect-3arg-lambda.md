# connect-3arg-lambda

Warns when using the 3-arg `QObject::connect` that takes a lambda.
The recommendation is to use the 4-arg overload, which takes a context object
so that the lambda isn't executed when the context object is deleted.

It's very common to use lambdas to connect signals to slots with different number
of arguments. This can result in a crash if the signal is emitted after the receiver
is deleted.
