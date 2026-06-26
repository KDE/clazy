# modernize-list-initialization

Suggests replacing `QStringList()` / `QList<T>()` followed by chained `<<` operators with initializer-list syntax.

This check prefers direct list initialization over temporary container construction and appending.

Example:

```cpp
QStringList myList = QStringList() << "abc" << "xyz" << "123";
```

With fixit:

```cpp
QStringList myList = {"abc", "xyz", "123"};
```
```

Example for function arguments:

```cpp
consumeList(QStringList() << "consume" << "meeee");
```

With fixit:

```cpp
consumeList({"consume", "meeee"});
```

Note: the check does not convert appends to a list produced by a function call, e.g. `produceList() << "more"` is not rewritten.
