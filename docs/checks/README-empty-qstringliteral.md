# empty-qstringliteral

Suggests to use an empty `QString` instead of an empty `QStringLiteral`.
The later causes unneeded code bloat.

You should use `QString()` instead of `QStringLiteral()` and `QStringLiteral("")`.

Note: Beware that `QString().isNull()` is `true` while both `QStringLiteral().isNull()` and `QStringLiteral("").isNull()` are `false`.
So be sure not to introduce subtle bugs when doing such replacements. In most cases it's simply a matter of using `isEmpty()` instead, which is `true` for all cases above.
