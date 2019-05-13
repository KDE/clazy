# qstring-unneeded-heap-allocations

Finds places with unneeded memory allocations due to temporary `QString`s.

Here's a summary of usages that allocate:

1. `QString s = "foo"; // Allocates, use QStringLiteral("foo") instead`

2. `QString s = QLatin1String("foo"); // Allocates, use QStringLiteral("foo") instead`

    2.1 `QString s = QLatin1String(""); // No allocation. QString is optimized for this case, so it's safe for empty literals`

3. `QString s = QStringLiteral("foo"); // No allocation`

4. `QString s = QString::fromLatin1("foo"); // Allocates, use QStringLiteral`

5. `QString s = QString::fromUtf8("foo"); // Allocates, use QStringLiteral`

6. `s == "foo" // Allocates, use QLatin1String`

7. `s == QLatin1String("foo") // No allocation`

8. `s == QStringLiteral("foo") // No allocation`

9. `QString {"append", "compare", "endsWith", "startsWith", "indexOf", "insert",`
   `         "lastIndexOf", "prepend", "replace", "contains" } // They all have QLatin1String overloads, so passing a QLatin1String is ok.`

10. `QString::fromLatin1("foo %1").arg(bar) // Allocates twice, replacing with QStringLiteral makes it allocate only once.`


#### Fixits

This check supports a fixit to rewrite your code. See the README.md on how to enable it.

#### Pitfalls

- `QStringLiteral` might make your app crash at exit if plugins are involved.
See:
<https://blogs.kde.org/2015/11/05/qregexp-qstringliteral-crash-exit> and
<http://lists.qt-project.org/pipermail/development/2015-November/023681.html>

- Also note that MSVC crashes when `QStringLiteral` is used inside initializer lists. For that reason no warning or fixit is emitted for this case unless you set an env variable:

        export CLAZY_EXTRA_OPTIONS="qstring-allocations-no-msvc-compat"
