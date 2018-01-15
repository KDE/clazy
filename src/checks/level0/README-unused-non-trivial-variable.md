# unused-non-trivial-variable

 Warns about unused Qt value classes.
 Compilers usually only warn when trivial classes are unused and don't emit warnings for non-trivial classes.

 This check has a whitelist of common Qt classes such as containers, `QFont`, `QUrl`, etc and warns for those too.

 See `UnusedNonTrivialType::isInterestingType(QualType t)` for a list of all types.

 It's possible to extend the whitelist with user types, by setting the env variable `CLAZY_UNUSED_NON_TRIVIAL_VARIABLE_WHITELIST`.
 It accepts a comma separate name of types.

 It's possible to disable the whitelist via exporting `CLAZY_EXTRA_OPTIONS=unused-non-trivial-variable-no-whitelist`,
 when this env variable is set clazy will warn for any unused non-trivial type. This will create many false positives,
 such as RAII classes, but still useful to run at least once on your codebase. When disabling the whitelist this way it's also possible
 to black list types, by setting a comma separated list of types to `CLAZY_UNUSED_NON_TRIVIAL_VARIABLE_BLACKLIST`
