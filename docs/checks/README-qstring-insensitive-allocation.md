# qstring-insensitive-allocation

Finds unneeded memory allocations such as
    `if (str.toLower().contains("foo"))` which you should fix as
    `if (str.contains("foo", Qt::CaseInsensitive))` to avoid the heap allocation caused by toLower().

Matches any of the following cases:
    `str.{toLower, toUpper}().{contains, compare, startsWith, endsWith}()`

#### Pitfalls
`Qt::CaseInsensitive` is different from `QString::toLower()` comparison for a few code points, but it
should be very rare: <http://lists.qt-project.org/pipermail/development/2016-February/024776.html>
