# unexpected-flag-enumerator-value

Checks `enum`s that are used as flag, for example:

```cpp
enum Foo {
    A = 0x1,
    B = 0x2,
    C = 0x4,
    D = 0x9 // Oops, typo
};
```

and checks whether all enumeration values are power of 2 or not.
