#include <QtCore/QDir>
#define MYSTRING "myDirPath"

void test()
{
    QDir dir;
    dir = "myStuff";

    QDir dir2;
    dir2 = MYSTRING;

    QDir dir3;
    dir3 = "my" "Stuff";

    QDir dir4;
    char *pathName = "myStuff";
    dir4 = pathName;

    QDir dir5;
    bool cond = true;
    dir5 = cond ? "mystuff" : "yourStuff";

    QDir dir6;
    dir6 = true ? (cond ? "path1" : "path2") : (cond ? "path3" : "path4");

    QDir::addResourceSearchPath("somePath");

}

