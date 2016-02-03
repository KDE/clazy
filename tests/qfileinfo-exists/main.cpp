#include <QtCore/QFileInfo>

int test1()
{
    if (QFileInfo("foo").exists()) // Warning
        return 0;

    if (QFileInfo::exists("foo")) // OK
        return 0;

    QFileInfo info;
    if (info.exists("foo")) // OK
        return 0;

    if (QFileInfo(QFile("foo")).exists()) // OK
        return 0;

    return 1;
}
