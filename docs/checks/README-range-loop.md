# range-loop

Finds places where you're using C++11 range-loops with non-const Qt containers (potential detach).

Fix it by marking the container const, or, since Qt 5.7, use `qAsConst()`:

#### Example

`for (auto i : qAsConst(list)) { ... }`

Also warns if you're passing structs with non-trivial copy-ctor or non-trivial dtor by value, use
const-ref so that the copy-ctor and dtor don't get called.

This warning can be unhelpful when the range type does not actually return a reference (for example `QJsonArray`), in which case using const-ref triggers a warning from `-Wrange-loop-analysis`.

#### Fixits
This check supports adding missing `&`, `const-&` or `qAsConst`.
