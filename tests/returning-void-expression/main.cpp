void foo();

void test() {
    return; // OK
    return foo(); // Warning
}

int test2()
{
    return 1; // OK
}
