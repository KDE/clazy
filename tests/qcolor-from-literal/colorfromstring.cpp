#include <QtGui/QColor>

extern void takingColor(QColor);

void test()
{
    takingColor(QColor::fromString("#123")); // Warning
}
