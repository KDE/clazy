# tr-non-literal

Finds calls to `QObject::tr()` with non literal argument.

Example: `tr(myStr.toUtf8());`

This usage of `tr()` might be incorrect because `lupdate` won't be able to pick up the strings for translation.
If the possible values have been marked with `QT_TR_NOOP` or `QT_TRANSLATE_NOOP`,
then this usage of `tr()` is actually fine, i.e. the clazy warning is a false-positive in that case.
