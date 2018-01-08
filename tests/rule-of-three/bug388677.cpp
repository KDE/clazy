#include <stdio.h>

class A
{
public:
    A()
    {
    }

    ~A()
    {
    }

    A(const A &) = delete;
    A& operator=(const A &) = delete;
};

class B : public A
{
public:
    B() : A()
    {
        i = 4;
    }

    ~B()
    {
        printf("%d\n", i);
    }

    int i;
};
