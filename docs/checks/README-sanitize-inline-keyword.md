# sanitize-inline-keyword

This check prints a warning if a member method, in an *exported* class (i.e. a class with
`visibility` attribute `default`), has the `inline` keyword on the definition but not
the declaration. While this code will typically compile, certain compilers (e.g. MinGW)
may show a warning in such cases, e.g.:
```
method redeclared without dllimport attribute after being referenced with dll linkage [-Werror]
```

At the time of writing this (2023-08-27), there is no easy way to suppress this warning,
and if warnings are treated as errors, the whole build can fail (e.g. on CI systems).

The fix is simple, the inline keyword should be specified for the declaration in-class, and
not the definition.

For more details see [this thread on the Qt Development mailing list](https://lists.qt-project.org/pipermail/development/2023-August/044351.html).

This check has a fixit that adds the `inline` keyword to the declaration, and removes
it from definition.

This check supports ignoring included files (i.e. only emit warnings for the current
file), when run via `clazy-standalone`. If you plan on using the fixits of this check,
it's recommended to use the `--ignore-included-files` option, to avoid having duplicate
fixes (which may result in the inline keyword being added more than once to the same
method).

### Limitations
- Removing the `inline` keyword from the definition leaves an extra space character,
  however clang-format can easily take care of that
- This check doesn't show a warning for `constexpr` methods or `function templates`,
  because both are implicitly inline, so it wouldn't cause the aforementioned problem

#### Example
```
// Exported/visible class
class __attribute__((visibility("default"))) MyClass
{
    int a() const;
};

inline int MyClass::a() const
{
    ....
}
```
