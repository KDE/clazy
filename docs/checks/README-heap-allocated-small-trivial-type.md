# heap-allocated-small-trivial-type

Warns when you're allocating small trivially copyable/destructible types on the heap.
Example:
```
    auto p = new QPoint(1, 1);
```

Unneeded memory allocations are costly.
