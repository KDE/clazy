#include <QtCore/QHash>
#include <QtCore/QString>

void test()
{
    QHash<const char*, int> hash1; // Warn
    QHash<char, int> hash2;
    QHash<char**, int> hash3;
    QHash<int, int> hash4;
    QHash<char32_t, int> hash5;
    QHash<const char32_t*, int> hash6;
}
