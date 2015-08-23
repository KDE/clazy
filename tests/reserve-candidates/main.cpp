#include <QtCore/QVector>

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

uint returns_uint() { return 0; }
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
}

void test_misc()
{
    QVector<int> v2;
    for (int i = 0; i < 10; ) {
        v2 << i; // OK
    }

}
