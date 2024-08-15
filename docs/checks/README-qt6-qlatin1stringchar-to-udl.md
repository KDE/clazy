# qt6-qlatin1stringchar-to-udl

This check's fixit aims to improve code readability by:
* Replacing `QLatin1StringView` (or `QLatin1String`, a type alias to `QLatin1StringView` in Qt6) when it wraps a string-literal, with the equivalent UDL (User Defined Literal) `""_L1`
* Replacing `QLatin1Char` when it wraps a char-literal with `u''`

`QLatin1StringView` and `QLatin1Char` in preprocessor macros are left as-is.

The fixit also adds `using namespace Qt::StringLiterals` directive at the top of source code (`.cpp`) files, if needed, after the last `#include` directive. This is acceptable for source code files, but not for header (`.h`) files, because the latter are included by other files, and that would leak this `using namesapce` directive, forcing it onto unsuspecting users. Typically this check should only be run on source code files.
