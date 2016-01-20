



static const char * const foo  = "1";
static const char * foo2 = "2";
const char * foo3 = "3"; // Warning

const char * const foo4 = "4";
static const char foo5[] ="5";
const char foo51[] ="5";

namespace {
    const char * foo6 = "6";
}

void test()
{
    const char * foo6 = "6";
}

extern const char *externFoo;
