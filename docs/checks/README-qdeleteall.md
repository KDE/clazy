# qdeleteall
Finds places where a call to `qDeleteAll()` has a redundant `values()` or `keys()` call.
Those calls create a temporary `QList` and allocate memory.

#### Example

    QSet<Cookies> set;

    // BAD: Unneeded container iteration and memory allocation to construct list of values
    qDeleteAll(set.values());

    // GOOD: No unneeded container iteration and memory allocation to construct list of values
    qDeleteAll(set);

#### Pitfalls

Very rarely you might be deleting a list of `QObject`s who's `destroyed()` signal is connected to some code
that modifies the original container. In the case of this contrived example iterating over the container copy is safer.
However, in this case it is probably better to iterate over an implicitly shared copy of the container. This makes
the code more explicit and ensures a copy is only made when modification actually happens.
