#include <QtCore/QTextStream>
#include <QtCore/QDir>
#include <QtCore/QMap>
#include <QtCore/QProcess>
#include <QtCore/QResource>
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

    QDir::addResourceSearchPath("somePath1");
    dir6.addResourceSearchPath("somePath2");

    QMap<QString, QString> m;
    m.insertMulti("foo", "bar");
    QList<QString> m_keys= m.uniqueKeys();
    QList<QString> m_list= m.values();
    QMap<QString, QString> mm;
    m.unite(mm);

    QProcess pp;
    pp.start("stringContainingACommandWithArguments");
    pp.execute("stringContainingACommandWithArguments");
    pp.startDetached("stringContainingACommandWithArguments");

    QResource rr;
    bool a_bool = rr.isCompressed();

    uint matchtype = 4;
    if (matchtype ==  Qt::MatchRegExp)
        matchtype = 0;

    QTextStream out;
    out << "blabla" << endl;
    out << hex << endl;

    QString a_string = "eeoaaoii";
    QString sep = "o";
    QStringList my_list =  a_string.split(sep, QString::KeepEmptyParts);
}

namespace Qt {
    void test_1() {
        uint matchtype = 4;
        if (matchtype ==  MatchRegExp)
            matchtype = 0;
        QTextStream out;
        out << "blabla" << QTextStreamFunctions::endl;
        out << QTextStreamFunctions::hex << QTextStreamFunctions::endl;

        QString a_string = "eeoaaoii";
        QString sep = "o";
        QStringList my_list =  a_string.split(sep, QString::KeepEmptyParts);
    }
}

