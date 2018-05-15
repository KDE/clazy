# reserve-candidates


Finds places that could use a `reserve()` call.
Whenever you know how many elements a container will hold you should reserve
space in order to avoid repeated memory allocations.

#### Trivial example missing reserve()

    QList<int> ages;
    // list.reserve(people.size());
    for (auto person : people)
        list << person.age();

Example where reserve shouldn't be used:

    QLost<int> list;
    for (int i = 0; i < 1000; ++i) {
        // reserve() will be called 1000 times, meaning 1000 allocations
        // whilst without a reserve the internal exponential growth algorithm would do a better job
        list.reserve(list.size() + 2);
        for (int j = 0; j < 2; ++j) {
            list << m;
        }
    }

#### Supported containers
`QVector`, `std::vector`, `QList`, `QSet` and `QVarLengthArray`

#### Pitfalls
Rate of false-positives is around 15%. Don't go blindly calling `reserve()` without proper analysis.
In doubt don't use it, all containers have a growth curve and usually only do log(N) allocations
when you append N items.
