# inefficient-qlist-soft

A less aggressive version of the **inefficient-qlist** check.

Finds `QList<T>` where `sizeof(T) > sizeof(void*)`. `QVector<T>` should be used instead.
Only warns if the container is a local variable and isn't passed to any method or returned,
unlike **inefficient-qlist**. This makes it easier to fix the warnings without concern about source and binary compatibility.
