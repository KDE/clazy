class MyClass
{
    inline int a();
    inline int b();
    int c();
    int d();
};

inline int MyClass::a()
{
    return 7;
}

int MyClass::b()
{
    return 3;
}

inline int MyClass::c()
{
    return 5;
}

// inline not the first Token on the line
int inline MyClass::d()
{
    return 9;
}

// Checks only exported classes, i.e. visibility "default"
class __attribute__((visibility("hidden"))) MyHiddenClass
{
    inline int a();
    inline int b();
    int c(); // Left as-is
};

inline int MyHiddenClass::a()
{
    return 7;
}

int MyHiddenClass::b()
{
    return 3;
}

inline int MyHiddenClass::c()
{
    return 5;
}
