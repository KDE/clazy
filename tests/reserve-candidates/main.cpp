#include <QtCore/QVector>
#include <QtCore/QHash>
#include <QtCore/QQueue>
#include <QtCore/QVarLengthArray>
#include <vector>



void local_vector()
{
    QVector<int> vec;
    for (int i = 0; i < 10; ++i) {
        vec << i;      // Test #1: Warning
        vec.append(i); // Test #2: Warning
    }

    QVector<int> vec1;
    vec1.reserve(10);
    for (int i = 0; i < 10; ++i) {
        vec1 << i;      // Test #3: No warning
        vec1.append(i); // Test #4: No warning
    }

    QVector<int> vec2;
    for (int i = 0; i < 10; ++i) {
        if (true) {
            vec1 << i;      // Test #5: No warning
            vec1.append(i); // Test #6: No warning
        }
    }

    for (int i = 0; i < 10; ++i) {
        QVector<int> v;
        v << 1; // OK
    }
}

void non_local_vector(QVector<int> &vec)
{
     // Test #7: No warning
    for (int i = 0; i < 10; ++i) {
        vec << i;
    }
}

bool returns_bool()
{
    return true;
}

unsigned int returns_uint() { return 0; }
int returns_int() { return 0; }
long long returns_long() { return 0; }

void test_complex_expressions()
{
    QVector<int> v;
    int a987r[10];
    for (int i = 0; a987r[i] == 1; ++i)
        v << i; // OK

    for (int i = 0; returns_bool(); ++i)
        v << i; // OK

    QVector<int> v2, v3;
    for (int i = 0; i < v2.size(); ++i)
        v3 << v3[i]; // Warning

    QVector<int> v4, v5;
    for (int i = 0; i < returns_uint(); ++i)
        v4 << v5[i]; // Warning

    QVector<int> v6, v7;
    for (int i = 0; i < returns_int(); ++i)
        v6 << v7[i]; // Warning

    QVector<int> v8, v9;
    for (int i = 0; i < returns_long(); ++i)
        v8 << v9[i]; // Warning
}

void test_nesting()
{
    QVector<int> v;

    while (returns_bool()) {
        for (int i = 0; i < 10; ++i)
            v << i; // OK
    }

    while (returns_bool()) {
        QVector<int> v2;
        for (int i = 0; i < 10; ++i)
            v2 << i; // Warning
    }

    QVector<int> v3;
    // Too many levels, this is ok, unless all of the cond expressions where literals but that's unlikely
    for (int i = 0; i < 10; ++i) {
        for (int i = 0; i < 10;  ++i) {
            for (int i = 0; i < 10; ++i) {
                for (int i = 0; i < 10; ++i) {
                    v3 << i; // OK
                }
            }
        }
    }

    QVector<int> a,b,c,d,e;
    foreach (int i, a)
        foreach (int i2, b)
                c << 1; // OK



}

void test_misc()
{
    QVector<int> v2;
    for (int i = 0; i < 10; ) {
        v2 << i; // OK
    }
}


class B
{
public:
    QVector<int> v;
};


class A
{
    A()
    {
        for (int i = 0; i < 10; ++i)
            v << i; // Warning

        for (int i = 0; i < 10; ++i)
            b.v << i; // OK
    }

    ~A()
    {
        for (int i = 0; i < 10; ++i)
            v << i; // Warning
    }

    void foo()
    {
        for (int i = 0; i < 10; ++i)
            v << i; // OK
    }

public:
    QVector<int> v;
    B b;
};

struct Node
{
    Node *next;
    int next2;
};

void testNode()
{
    QVector<int> v, v2;
    Node *node;
    for (int i = 0; i < 10; node = node->next) // OK
        v << i;

    for (int i = 0; i < 10; i = i + 1) v << i; // Warning

    for (auto it = v2.cbegin(), e = v2.cend(); it != e; ++it)
        v << 0; // Warning

    for (auto it = v2.cbegin(), e = v2.cend(); it != e; it = it + 1)
        v << 0; // Warning

    for (int i = 0; i < 10; i = node->next2)
        v << i; // OK


    for (int i = 0; i < 10; ++i) {
        v << 1;  // Warning
        v2 << 1; // Warning
    }

    for (int i = 0; i < 10; i = node->next2) {
        v.push_back(1);  // OK
        v.push_back(1);  // OK
        v2.push_back(1); // OK
        v2.push_back(1); // OK
    }
}


struct testCTOR
{
    testCTOR()
    {
        Node *node;
        for (int i = 0; i < 10; i = node->next2) {
            m_v << 1; // OK
        }
    }

    QVector<int> m_v;
};


void moreStuff()
{
    QVector<int> v;
    for (int i = 0; ; ++i)
        v.push_back(1);

    QHash<int,int> h;
    QHashIterator<int,int> it(h);
    while (it.hasNext()) { // Ok
        v.push_back(1);
    }
}


void rangeLoop()
{
    QVector<int> v1, v2;
    for (auto i : v1)
        v2.push_back(i);
}

struct Foo
{
    Foo ip() const;
    QList<Foo> addressEntries;
};

void testNesting2()
{
    QList<Foo> result;
    QList<Foo> privs;
    foreach (const Foo &p, privs)
        foreach (const Foo &entry, privs)
            result += entry.ip();

    QVector<int> v2;
    for (int u = 0; u < 10; ++u) {
        for (int n = 0; n < 10; ++n) {
            v2.append(1);
        }
    }
}


void bug362943()
{
    QVector<int> vect;
    QQueue<int> q;
    q.reserve(10);
    for (const int i: vect) {
        q << i;
    }
}

void bug367484()
{
    QVarLengthArray<int> array;
    for (int i = 0; i < 10; ++i) {
        array.append(i); // OK
    }
}

void test_std_vector()
{
    std::vector<int> ints;
    for (int i = 0; i < 10; ++i)
        ints.push_back(i);
}
