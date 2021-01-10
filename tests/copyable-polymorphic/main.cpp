

struct ValueClass // OK
{
    int m;
};

struct DerivedValueClass : public ValueClass // OK
{
    int m;
};

struct PolymorphicClass1 // OK, not copyable
{
    virtual void foo();
    PolymorphicClass1(const PolymorphicClass1 &) = delete;
    PolymorphicClass1& operator=(const PolymorphicClass1 &) = delete;
};

struct PolymorphicClass2 // OK, not copyable
{
    virtual void foo();
private:
    PolymorphicClass2(const PolymorphicClass2 &);
    PolymorphicClass2& operator=(const PolymorphicClass2 &);
};

struct PolymorphicClass3 // OK, not copyable
{
    virtual void foo();
    PolymorphicClass3(const PolymorphicClass3 &) = delete;
private:
    PolymorphicClass3& operator=(const PolymorphicClass3 &) = delete;
};

struct PolymorphicClass4 // Warning, copyable
{
    virtual void foo();
    PolymorphicClass4(const PolymorphicClass4 &);
    PolymorphicClass4& operator=(const PolymorphicClass4 &);
};

struct PolymorphicClass5 // Warning, copyable
{
    virtual void foo();
};

struct DerivedFromNotCopyable : public PolymorphicClass3 // OK, not copyable
{
};

struct DerivedFromCopyable : public PolymorphicClass4 // Warning, copyable
{
};
