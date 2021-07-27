
// Enum is too small, won't be checked
enum TooSmall {
    FooTooSmall = 1,            // Ok
    BarTooSmall = 2,            // Ok
    LalaTooSmall = 4,            // Ok
};


enum TEST_FLAGS {
    Foo = 1,            // Ok
    Bar = 2,            // Ok
    Both = Foo | Bar,   // OK
    Something = 3       // Warn
};

// Copied from Qt for testing purposes
enum AlignmentFlag {
    AlignLeft = 0x0001,
    AlignLeading = AlignLeft,
    AlignRight = 0x0002,
    AlignTrailing = AlignRight,
    AlignHCenter = 0x0004,
    AlignJustify = 0x0008,
    AlignAbsolute = 0x0011, // Warn
    AlignHorizontal_Mask = AlignLeft | AlignRight | AlignHCenter | AlignJustify | AlignAbsolute,

    AlignTop = 0x0020,
    AlignBottom = 0x0040,
    AlignVCenter = 0x0080,
    AlignBaseline = 0x0100,
    // Note that 0x100 will clash with Qt::TextSingleLine = 0x100 due to what the comment above
    // this enum declaration states. However, since Qt::AlignBaseline is only used by layouts,
    // it doesn't make sense to pass Qt::AlignBaseline to QPainter::drawText(), so there
    // shouldn't really be any ambiguity between the two overlapping enum values.
    AlignVertical_Mask = AlignTop | AlignBottom | AlignVCenter | AlignBaseline,

    AlignCenter = AlignVCenter | AlignHCenter
};

// No Warning here, all consecutive values
enum {
    RED = 1,
    GREEN = 2,
    BLUE = 3,
    BLACK = 4,
    WHITE = 5,
};

#include <QtCore/qtypeinfo.h>

class FooBar {
    public:
        int m = 0;
};
Q_DECLARE_TYPEINFO(FooBar, Q_RELOCATABLE_TYPE);

enum WithMasks
{
    V1 = 1,
    V2 = 2,
    V4 = 4,
    V8 = 8,
    Mask = 0x0000FFFF
};


enum class WithZero {
    Zero = 0,
    Foo = 1,
    Bar = 2,
    Four = 4,
    Eight = 8,
};
