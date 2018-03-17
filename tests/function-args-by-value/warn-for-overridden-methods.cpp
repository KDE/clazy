struct Trivial {};

class BaseWithVirtuals
{
public:
    virtual void virtualMethod1(const Trivial &) {}; // Warn
    virtual void virtualMethod2(const Trivial &) {}; // Warn
    void nonVirtualMethod(const Trivial &) {}; // Warn
};

class DerivedWithVirtuals : BaseWithVirtuals {
public:
    void virtualMethod1(const Trivial &) override {}; // Warn
    void virtualMethod2(const Trivial &) {}; // Warn
    void nonVirtualMethod(const Trivial &) {}; // Warn
};
