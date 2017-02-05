# container-inside-loop

Finds places defining containers inside loops.
Defining them outside the loop and using `resize(0)` will save memory allocations.

#### Example

    // This will allocate memory at least N times:
    for (int i = 0; i < N; ++i) {
        QVector<int> v;
        (...)
        v.append(bar);
        (...)
    }

    // This will reuse previously allocated memory:
    QVector<int> v;
    for (int i = 0; i < N; ++i) {
        v.resize(0); // resize(0) preserves capacity, unlike QVector::clear()
        (...)
        v.append(bar);
        (...)
    }

#### Supported containers

`QList`, `QVector` and `std::vector`
