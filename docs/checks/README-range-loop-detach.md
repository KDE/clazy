# range-loop-detach

Finds places where you're using C++11 range-loops with non-const Qt containers (potential detach).

Fix it by marking the container const, or using `qAsConst`/`std::as_const`

#### Example

`for (auto i : qAsConst(list)) { ... }`
`for (auto i : std::as_const(list)) { ... }`

#### Fixits
This check supports adding missing `qAsConst` or `std::as_const` when the file is compiled with C++17.
