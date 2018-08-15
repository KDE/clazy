# empty-qstringliteral

Suggests to use an empty `QString` instead of an empty `QStringLiteral`.
The later causes unneeded code bloat.

You should use `QString()` instead of `QStringLiteral()` and use `QString("")` instead of `QStringLiteral("")`.
