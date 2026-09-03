# cache-model-rolenames

# role-names-cache

Warns when `QAbstractItemModel::roleNames()` constructs its `QHash<int, QByteArray>` on every call.

Role names are typically static for the lifetime of a model, so the hash should be cached instead of rebuilt repeatedly. Prefer a `static const` local hash, or a member when the role names need to be instance-specific.

```cpp
QHash<int, QByteArray> roleNames() const override
{
    static const QHash<int, QByteArray> roles{
        // roles...
    };
    return roles;
}
```
