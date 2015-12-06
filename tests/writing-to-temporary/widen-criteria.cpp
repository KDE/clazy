struct Foo
{
    void move(int v) { }
};

Foo getFoo() { return Foo(); }

void test()
{
    getFoo().move(1);
}
