#include <stdio.h>

class A
{
public:
    A() {}
    ~A() {}

    A(const A &) {}
    A& operator=(const A &) = delete;
};

int main(int, char *[])
{
    A a;
}
