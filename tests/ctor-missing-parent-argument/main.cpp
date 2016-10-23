#include <QtCore/QObject>

class Test : public QObject // Warn
{
public:
    Test();
};

class Test2 : public Test // Warn
{
public:
    Test2();
};

class Test3 // OK
{
public:
    Test3();
};

class Test4 : public Test // OK
{
public:
    Test4();
    Test4(QObject*); //  Not common, but lets not warn
};

class Test5 : public Test // Warn
{
public:
    Test5(const QObject*);
};

class QObject; // OK
