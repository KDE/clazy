# compare-member-check

Warns when comparison operators don't use all member variables of a class.
This helps prevent bugs by ensuring consistent comparisons, particularly important
when using containers or sorting.

Example:
```cpp
class ExampleClass {
public:
    int a;
    int b;
    int c;

    bool operator>(const ExampleClass &other) const {
        return a > other.a || b > other.b; // Warning: 'c' is not used
    }

    // Fixed using std::tie:
    bool operator<(const ExampleClass &rhs) const {
        return std::tie(a, b, c) < std::tie(rhs.a, rhs.b, rhs.c); // OK
    }
};
```

The check will not warn for:
- Classes without member variables
- Qt's meta-object system methods
- Template classes

For implementing comparisons, prefer using `std::tie` as it provides a less error-prone
way to compare multiple members. See `std::tie` documentation for more information.
