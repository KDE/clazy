# unused-non-trivial-variable

 Warns about unused Qt value classes.
 Compilers usually only warn when trivial classes are unused and don't emit warnings for non-trivial classes.

 This check has a whitelist of common Qt classes such as containers, `QFont`, `QUrl`, etc and warns for those too.

 See `UnusedNonTrivialType::isInterestingType(QualType t)` for a list of all types.

 It's possible to disable the whitelist via exporting `CLAZY_EXTRA_OPTIONS=unused-non-trivial-variable-no-whitelist`,
 when this env variable is set clazy will warn for any unused non-trivial type. This will create many false positives,
 such as RAII classes, but still useful to run at least once on your codebase.
