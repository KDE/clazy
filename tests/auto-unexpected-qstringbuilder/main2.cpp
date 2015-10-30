#include <QtCore/QString>

// No test without string builder macro
void test2()
{
    const auto s2 = "tests/" + QString::fromLatin1("foo"); // OK
    const QString s3 = "tests/" + QString::fromLatin1("foo"); // OK
}
