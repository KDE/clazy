# qhash-with-char-pointer-key

Finds cases of `QHash<const char *, T>`. It's error-prone as the key is just compared by the address of the string literal and not the value itself.

This check is disabled by default as there are valid uses-cases.
