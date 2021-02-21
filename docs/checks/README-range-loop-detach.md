# range-loop-detach

Finds places where you're using C++11 range-loops with non-const Qt containers (potential detach).

Fix it by marking the container const, or, since Qt 5.7, use `qAsConst()`:

#### Example

`for (auto i : qAsConst(list)) { ... }`

#### Fixits
This check supports adding missing `qAsConst`.
