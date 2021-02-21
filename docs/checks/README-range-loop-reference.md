# range-loop-reference

Finds places where you're using C++11 range-loops with non-trivial types by value so that the copy-ctor and dtor don't get called.

Fix it by adding a (const) reference to the range declaration:

#### Example

`for (const auto &i : list) { ... }`

#### Fixits
This check supports adding missing `&` or `const-&` (if the range declaration is already `const`).
