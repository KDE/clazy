#include <QtCore/QCoreApplication>







struct NonPod
{
    NonPod() {}
    ~NonPod() {}
    int a;
};

struct Pod
{
    int a;
};


static Pod p;
static NonPod p2;
static NonPod* p3;
static int p4;

void foo() {}
Q_COREAPP_STARTUP_FUNCTION(foo) // OK, it's blacklisted
