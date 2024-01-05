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
    virtual void virtualMethod1(const NonTrivial&) {}; // Warn
    virtual void virtualMethod2(const NonTrivial&) {}; // Warn
    void nonVirtualMethod(const NonTrivial&) {}; // Warn
};

class DerivedWithVirtuals : BaseWithVirtuals {
public:
    void virtualMethod1(const NonTrivial&) override {}; // Warn
    void virtualMethod2(const NonTrivial&) {}; // Warn
    void nonVirtualMethod(const NonTrivial&) {}; // Warn
};
