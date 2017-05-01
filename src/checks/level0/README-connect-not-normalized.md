# connect-not-normalized

Warns when the contents of `SIGNAL()`, `SLOT()`, `Q_ARG()` and `Q_RETURN_ARG()` are not normalized.

Using normalized signatures allows to avoid unneeded memory allocations.

For signals and slots it only warns for `connect` statements, not `disconnect`, since it only
impacts the performance of the former.

See `QMetaObject::normalizedSignature()` for more information.
