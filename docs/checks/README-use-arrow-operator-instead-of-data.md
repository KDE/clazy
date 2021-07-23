# use-arrow-operator-instead-of-data

Finds code such as:

```
QScopedPointer<T> ptr...;
ptr.data()->someFunc();
```

and suggests to use the arrow operator(`->`) directly instead:

```
ptr->someFunc();
```
