# missing-typeinfo

Suggests usage of `Q_PRIMITIVE_TYPE` or `Q_MOVABLE_TYPE` in cases where you're using `QList<T>` and `sizeof(T) > sizeof(void*)`
or using `QVector<T>`, unless they already have a type info classification.

See `Q_DECLARE_TYPEINFO` in Qt documentation for more information.
