# mutable-container-key

Looks for `QMap` or `QHash` having key types which can be modified due to external factors.
The key's value should never change, as it's needed for sorting or hashing, but with some types, such as non-owning smart pointers it might happen.
The supported key types are: `QPointer`, `QWeakPointer`, `weak_ptr` and `QPersistentModelIndex`.
