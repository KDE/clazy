struct Trivial
{
};

struct NonTrivial
{
    ~NonTrivial();
};

void test()
{
    auto l1 = [](const Trivial &) { return 0; };
    auto l2 = [](Trivial) { return 0; };
    auto l3 = [](const NonTrivial &) { return 0; };
    auto l4 = [](NonTrivial) { return 0; };
}
