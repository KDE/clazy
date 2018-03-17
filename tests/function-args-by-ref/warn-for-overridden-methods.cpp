struct NonTrivial {
    NonTrivial() {}
    NonTrivial(const NonTrivial &) {}
    void constFunction() const {};
    void nonConstFunction() {};
    int a;
};

class BaseWithVirtuals
{
public:
    virtual void virtualMethod1(NonTrivial) {}; // Warn
    virtual void virtualMethod2(NonTrivial) {}; // Warn
    void nonVirtualMethod(NonTrivial) {}; // Warn
};

class DerivedWithVirtuals : BaseWithVirtuals {
public:
    void virtualMethod1(NonTrivial) override {}; // Warn
    void virtualMethod2(NonTrivial) {}; // Warn
    void nonVirtualMethod(NonTrivial) {}; // Warn
};
