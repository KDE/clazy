# heap-allocated-small-trivial-type

Warns when you're allocating small trivially copyable/destructible types on the heap.
Example:
```
    auto p = new QPoint(1, 1);
    /// ... p just used locally in the scope
```

Unneeded memory allocations are costly. Make sure there's no change in behaviour
before fixing these warnings. This check is not enabled by default since there's
a certain amount of known false-positives.
