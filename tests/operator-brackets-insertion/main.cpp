#include <QtCore/QHash>
#include <QtCore/QMap>
#include <QtCore/QString>

QMap<QString, int> getMap()
{
    return {};
}

QHash<int, int> getHash()
{
    return {};
}

void warnOnReadLikeUses()
{
    if (getMap()["key"] == 0) { }

    const auto value = getHash()[42];

    if (getHash()[7] > 0) { }
}

void noWarningOnWriteLikeUses()
{
    QMap<QString, int> map;
    map["key"] = 1;
    map["key"] += 2;
    ++map["key"];

    QHash<int, int> hash;
    hash[42] = 3;
    hash[42] += 5;
    ++hash[42];

    const auto x = map.value("key");

    const auto y = hash.value(42);
}
