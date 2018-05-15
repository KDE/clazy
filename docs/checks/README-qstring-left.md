# qstring-left

Finds places where you're using `QString::left(1)` instead of `QString::at(0)`.
The later form is cheaper, as it doesn't deep-copy the string.

There's however another difference between the two: `left(1)` will return an empty
string if the string is empty, while `QString::at(0)` will assert. So be sure
that the string can't be empty, or add a `if (!str.isEmpty()` guard, which is still
faster than calling `left()` for the cases which deep-copy.
