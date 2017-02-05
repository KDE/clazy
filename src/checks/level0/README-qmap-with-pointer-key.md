# qmap-with-pointer-key

Finds cases where you're using `QMap<K,T>` and K is a pointer.

`QMap` has the particularity of sorting it's keys, but sorting by memory
address makes no sense.
Use `QHash` instead, which provides faster lookups.
