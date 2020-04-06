#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QMap>

QMap<int, int> getMap() {
    return {};
}


struct Foo {
    int *m = nullptr;
};

void test()
{
    QMap<int,int> map;
    map.insert(0, 0);
    int &a = map[0]; // Warn
    int b = map[0];

    int &c = b; //  OK

    int &d = getMap()[0]; // Warn
    //Foo f;
    //f.m = &map[0];
    int aa;
    map[0] = aa;
}

