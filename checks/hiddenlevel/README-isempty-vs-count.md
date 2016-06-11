# isempty-vs-count

Finds places where you're using `Container::count()` or `Container::size()` instead of `Container::isEmpty()`.
Although there's no performance benefit, `isEmpty()` provides better semantics.

#### Example
```
QList<int> foo;
...
if (foo.count()) {}
```
