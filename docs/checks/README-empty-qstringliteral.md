# empty-qstringliteral

Suggests to use `QLatin1String("")` instead of `QStringLiteral()` and `QStringLiteral("")`.
`QStringLiteral` should only be used where it would reduce memory allocations.

Note that, counterintuitively, both `QStringLiteral().isNull()` and `QStringLiteral("").isNull()` are `false`,
so do use exactly `QLatin1String("")` and not `QLatin1String()`, in both cases.

If in your code `isNull()` and `isEmpty()` are interchangeable, then simply use `QString()`.
