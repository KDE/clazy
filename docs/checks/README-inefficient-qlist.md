# inefficient-qlist

Finds `QList<T>` where `sizeof(T) > 64`. `QVector<T>` should be used instead.

This check uses 64 as the limit instead of `sizeof(void*)` so that it gives the same
results in 32-bit platforms. Making your 32-bit code portable to 64-bit without pessimizations.

This is a very noisy check and hence disabled by default. See **inefficient-qlist-soft** for a more useful check.
