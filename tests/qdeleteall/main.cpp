// clang+++ test.cpp -I /usr/include/qt/ -fPIC -lQt5Core -c

#include <QtCore/QHash>
#include <QtCore/QMap>
#include <QtCore/QSet>
#include <QtCore/QString>

QSet<QString *> values()
{
    QSet<QString *> s;
    return s;
}

QSet<QString *> keys()
{
    QSet<QString *> s;
    return s;
}

struct Foo {
    QSet<QString *> values()
    {
        QSet<QString *> s;
        return s;
    }

    QSet<QString *> doSomethingWithValues(const QList<QString*> &)
    {
        QSet<QString *> s;
        return s;
    }
};

int main()
{
    QSet<QString *> s;
    qDeleteAll(s);
    qDeleteAll(s.begin(), s.end());
    qDeleteAll(s.values()); // warning

    QHash<int, QString *> h;
    qDeleteAll(h);
    qDeleteAll(h.begin(), h.end());
    qDeleteAll(h.values()); // warning

    QMap<int*, QString *> m;
    qDeleteAll(m);
    qDeleteAll(m.begin(), m.end());
    qDeleteAll(m.values()); // warning

    QMultiHash<int, QString *> mh;
    qDeleteAll(mh);
    qDeleteAll(mh.begin(), mh.end());
    qDeleteAll(mh.values()); // warning

    QMultiMap<int, QString *> mm;
    qDeleteAll(mm);
    qDeleteAll(mm.begin(), mm.end());
    qDeleteAll(mm.values()); // warning

    qDeleteAll(values());  // ok

    Foo foo;
    qDeleteAll(foo.values());  // ok
    qDeleteAll(foo.doSomethingWithValues(h.values()));  // ok

    qDeleteAll(m.keys()); // warning
    qDeleteAll(keys()); // ok

    qDeleteAll(h.values(1)); // warning

}
