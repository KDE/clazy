#include <cstdio>

struct Color
{
    unsigned int r;
    unsigned int g;
    unsigned int b;
    unsigned int opacity;

    Color &operator=(const Color &c) = default;
    bool operator==(const Color &c) const = default;
    Color(const Color &c) = default;
    Color() = default;
};
struct Color2
{
    unsigned int r;
    unsigned int g;
    unsigned int b;
    unsigned int opacity;

    Color2 &operator=(const Color2 &c) {
        r=c.r;
        g=c.g;
        b=c.b;
        opacity =c.opacity;
        return *this;
    }
    bool operator==(const Color2 &c) const {
        return c.b == b;
    }
};

int main()
{
    Color c;
    c.r = 3;

    Color c2;
    c2 = c;

    Color c3(c2);


    Color2 otherType;
    Color2 otherType2;

    otherType2 = otherType;

    printf("C2.r %d\n", c2.r);
    printf("C3.r %d\n", c3.r);
}

