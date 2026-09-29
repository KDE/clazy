# operator-brackets-insertion

Finds read-like access to `QMap::operator[]()` and `QHash::operator[]()` that unexpectedly inserts a value when the key is missing.

This is not a general `operator[]` warning; it targets the specific side effect of Qt associative containers: reading through `map[key]` creates the entry if it does not exist yet. In many cases, `value()` or `contains()` is the intended call.

Examples:

```cpp
QMap<QString, int> map;
if (map["key"] == 0) {
    // Warning: this inserts "key" if it was missing
}

QHash<int, int> hash;
int value = hash[42];
// Warning: this inserts 42 into the hash if it was absent
```

The following are fine:

```cpp
map["key"] = 1;
map["key"] += 2;

if (map.contains("key") && map["key"] == 0) {
}

int v = map.value("key");
```

This check is intentionally conservative and only targets the Qt map/hash cases where `operator[]` has the insertion side effect.
