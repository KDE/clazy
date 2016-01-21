#include <QtCore/QCoreApplication>







struct NonPod
{
    NonPod() {}
    ~NonPod() {}
    int a;
};

struct NonPod2
{
    NonPod2() {}
};

struct NonPod3
{
    ~NonPod3() {}
};


struct Pod
{
    int a;
};


static Pod p;
static NonPod p1;
static NonPod2 p2;
static NonPod3 p3;
static NonPod* p4;
static int p5;

void foo() {}
Q_COREAPP_STARTUP_FUNCTION(foo) // OK, it's blacklisted

struct NonPodButOk
{
    NonPodButOk() = default;
    NonPodButOk(const NonPodButOk &) {}
};

static NonPodButOk p6; // OK, both called ctor and dtor are trivial

NonPod p7; // OK, it's not static, might be used somewhere else

struct HasConstExprCtor
{
    constexpr HasConstExprCtor(int i) : m_i(i) {}
    const int m_i;
};

constexpr static HasConstExprCtor p8(1);
static HasConstExprCtor p9(1);
