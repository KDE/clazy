#include <vector>

struct A
{
    bool getA(int &a, int*, const int &b) const { return 1; };
};

struct QDir
{
    int mkdir() const { return 1;}
};

struct Large
{
    int a[1000];
};

const Large foo123(int f)
{
    if (true)
        return Large();

    Large l;
    return l;
}


void bar(std::vector<int> &v)
{
    for (int i= 0; i<12; ++i)
        v.push_back(1);
}

void bar2()
{
    std::vector<int> v111;
    for (int i= 0; i<12; ++i)
        if (true)
            v111.push_back(1);
}

int main()
{
    std::vector<A> vec;
    for (int i=0 ;i<5; ++i) {
        vec.push_back(A());
    }

    for (int i=0 ;i<5; ++i)
        vec.push_back(A());


    while (true)
        vec.push_back(A());

    do {
        vec.push_back(A());
    } while(true);
}

