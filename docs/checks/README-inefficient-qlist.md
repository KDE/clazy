# inefficient-qlist

Finds `QList<T>` where `sizeof(T) > sizeof(void*)`. `QVector<T>` should be used instead.

This is a very noisy check and hence disabled by default. See **inefficient-qlist-soft** for a more useful check.
