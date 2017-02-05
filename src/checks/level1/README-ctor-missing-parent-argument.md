# ctor-missing-parent-argument

Warns when `QObject` derived classes don't have at least one CTOR receiving a QObject.

This is an attempt to catch classes which are missing the parent argument.
It doesn't have false-positives, but might miss true-positives. For example,
if you have a CTOR which receives a `QObject* but then forget to pass it to the
base CTOR.
