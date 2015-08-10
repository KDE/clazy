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
}

void non_local_vector(QVector<int> &vec)
{
     // Test #7: No warning
    for (int i = 0; i < 10; ++i) {
        vec << i;
    }
}
