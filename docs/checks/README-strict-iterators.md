# strict-iterators

Warns when `iterator` objects are implicitly cast to `const_iterator`.
This is mostly equivalent to passing -DQT_STRICT_ITERATORS to the compiler.
This prevents detachments but also caches subtle bugs such as:

    QHash<int, int> wrong;
    if (wrong.find(1) == wrong.cend()) {
        qDebug() << "Not found";
    } else {
        qDebug() << "Found"; // find() detached the container before cend() was called, so it prints "Found"
    }

    QHash<int, int> right;
    if (right.constFind(1) == right.cend()) {
        qDebug() << "Not found"; // This is correct now !
    } else {
        qDebug() << "Found";
    }
