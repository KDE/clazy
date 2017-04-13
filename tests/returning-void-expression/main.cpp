void foo();

void test() {
    return; // OK
    return foo(); // Warning
}

int test2()
{
    return 1; // OK
}

int test3()
{
    return {}; // OK (bug #378677)
}

template <typename T>
T test4()
{
    return {}; // OK (bug #378677)
}

template <typename T>
void test5()
{
    return foo(); // Warning
}
