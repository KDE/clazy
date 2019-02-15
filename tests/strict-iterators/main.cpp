#include <QtCore/QString>
#include <QtCore/QMap>
#include <QtCore/QHash>
#include <QtCore/QVector>
#include <QtCore/QList>
#include <QtCore/QSet>
#include <QtCore/QVarLengthArray>
#include <vector>
void testTypedefedIterators()
{
    QVector<int> vec;
    QString str;
    QVarLengthArray<int, 5> vla;

    QVector<int>::const_iterator it1 = vec.begin(); // Warning
    QString::const_iterator it7 = str.begin(); // Warning
    QString::const_iterator it8 = str.cbegin(); // OK
    QVarLengthArray<int, 5>::const_iterator it9 = vla.begin(); // Ok, not shared
    QVarLengthArray<int, 5>::const_iterator it10 = vla.cbegin(); // OK

    // These are not implemented yet. Can't figure it out.. the QualType doesn't have the typedef information, only T*..

    if (vec.cend() == vec.end()) {} // OK
    if (vec.end() == vec.cend()) {} // Warning
    if (vec.cend() == vec.cend()) {} // OK
    if (vec.end() == vec.end()) {} // OK
    str.begin() == str.cbegin(); // Warning
    vla.begin() == vla.cbegin(); // OK, not shared
}

void test()
{
    QMap<int,int> map;
    QHash<int,int> hash;
    QList<int> list;
    QSet<int> set;


    QList<int>::const_iterator it2 = list.begin(); // Warning
    QHash<int,int>::const_iterator it3 = hash.begin(); // Warning
    QHash<int,int>::const_iterator it4 = hash.cbegin(); // OK
    QMap<int,int>::const_iterator it5 = map.begin(); // Warning
    QMap<int,int>::const_iterator it6 = map.cbegin(); // OK
    QSet<int>::const_iterator it11 = set.begin(); // Warning
    it11 = set.cbegin(); // OK

    hash.begin() == hash.cbegin(); // Warning
    list.begin() == list.cbegin(); // Warning
    set.begin() == set.cbegin(); // Warning
    map.begin() == map.cbegin(); // Warning

    hash.begin() == hash.begin(); // OK
    list.begin() == list.begin(); // OK
    set.begin() == set.begin(); // OK
    map.begin() == map.begin(); // OK

    hash.cbegin() == hash.cbegin(); // OK
    list.cbegin() == list.cbegin(); // OK
    set.cbegin() == set.cbegin(); // OK
    map.cbegin() == map.cbegin(); // OK

    QHash<int,int>::const_iterator it12 = QHash<int,int>::const_iterator(hash.begin()); // OK
}

void test2()
{
    QVector<int> v;
    v.erase(std::remove_if(v.begin(), v.end(), [](int){ return false; }),
            v.end()); // OK

    QVarLengthArray<int, 5> vla;
    vla.erase(std::remove_if(vla.begin(), vla.end(), [](int){ return false; }),
              vla.end()); // OK, not implicit shared
}


void testStdVector()
{
    std::vector<int> v;
    std::vector<int>::const_iterator it = v.begin(); // OK
}


struct Bar {
    bool foo() const
    {
        return mFooIt != mFoo.end(); // OK, since mFooIt is a member, it won't make anything detach
    }
    QVector<int> mFoo;
    QVector<int>::iterator mFooIt;
};

