# container-anti-pattern

Finds when temporary containers are being created needlessly.
These cases are usually easy to fix by using iterators, avoiding memory allocations.

Matches code like:

    {QMap, QHash, QSet}.values().*
    {QMap, QHash}.keys().*
    {QVector, QSet}.toList().*
    QList::toVector().*
    QSet::intersect(other).isEmpty()
    for (auto i : {QHash, QMap}.values()) {}
    foreach (auto i, {QHash, QMap}.values()) {}

#### Example

    set.toList()[0]; // use set.constFirst() instead
    hash.values().size(); // Use hash.size() instead
    hash.keys().contains(); // Use hash.contains() instead
    hash.values().contains(); // Use std::find(hash.cbegin(), hash.cend(), myValue) instead
    map.values(k).foo ; // Use QMap::equal_range(k) instead
    for (auto i : hash.values()) {} // Iterate the hash directly instead: for (auto i : hash) {}
    QSet::intersect(other).isEmpty() // Use QSet::intersects() instead, avoiding memory allocations and iterations, since Qt 5.6
