# qdeleteall
Finds places where a call to `qDeleteAll()` has a redundant `values()` or `keys()` call.
Those calls create a temporary `QList<int>` and allocate memory.

#### Example

    QSet<Cookies> set;

    // BAD: Unneeded container iteration and memory allocation to construct list of values
    qDeleteAll(set.values());

    // GOOD: Unneeded container iteration and memory allocation to construct list of values
    qDeleteAll(set);

#### Pitfalls

Very rarely you might be deleting a list of `QObject`s who's `destroyed()` signal is connected to some code
that modifies the original container. In the case of this contrived example iterating over the container copy is safer.
