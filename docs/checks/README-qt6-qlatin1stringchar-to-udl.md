# qt6-qlatin1stringchar-to-udl


This check's fixit aim to improve code readability by:
* Replacing `QLatin1StringView` (or `QLatin1String`, a type alias to `QLatin1StringView` in Qt6) when it wraps a string-literal, with the equivalent UDL (User Defined Literal) `""_L1`
* Replacing `QLatin1Char` when it wraps a char-literal with `u''`

`QLatin1StringView` and `QLatin1Char` in preprocessor macros are left as-is.
