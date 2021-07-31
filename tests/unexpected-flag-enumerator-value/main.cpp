
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
enum Consecutive {
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

enum RelationFlag {
    Label         = 0x00000001,
    Labelled      = 0x00000002,
    Controller    = 0x00000004,
    Controlled    = 0x00000008,
    AllRelations  = 0xffffffff
};

enum class WithZero {
    Zero = 0,
    Foo = 1,
    Bar = 2,
    Four = 4,
    Eight = 8,
};

enum class AlsoConsecutive {
    RED = 1,
    GREEN = 2,
    BLUE = 3,
    BLACK = 4,
    WHITE = 5,
    LAST = WHITE
};

enum class WithNegative
{
    V1 = 1,
    V2 = 2,
    V4 = 4,
    V8 = 8,
    VFoo = -1
};

enum Filter {
    Dirs        = 0x001,
    Files       = 0x002,
    Drives      = 0x004,
    NoSymLinks  = 0x008,
    AllEntries  = Dirs | Files | Drives,// Because of this -1, the type of this enum becomes signed and hence BinaryOp | doesn't need any implicit casts
    TypeMask    = 0x00f,

    NoFilter = -1
};

enum ComponentFormattingOption : unsigned int {
    PrettyDecoded = 0x000000,
    EncodeSpaces = 0x100000,
    EncodeUnicode = 0x200000,
    EncodeDelimiters = 0x400000 | 0x800000,
    EncodeReserved = 0x1000000,
    DecodeReserved = 0x2000000,
    // 0x4000000 used to indicate full-decode mode

    FullyEncoded = EncodeSpaces | EncodeUnicode | EncodeDelimiters | EncodeReserved,
    FullyDecoded = FullyEncoded | DecodeReserved | 0x4000000
};
enum MarkdownFeature {
    MarkdownNoHTML = 0x0020 | 0x0040,
    MarkdownDialectCommonMark = 0,
    MarkdownDialectGitHub = 0x0004 | 0x0008 | 0x0400 | 0x0100 | 0x0200 | 0x0800 | 0x4000
};
enum Feature {
    FeatureCollapseWhitespace =       0x0001,
    FeaturePermissiveATXHeaders =     0x0002,
    FeaturePermissiveURLAutoLinks =   0x0004,
    FeaturePermissiveMailAutoLinks =  0x0008,
    FeatureNoIndentedCodeBlocks =     0x0010,
    FeatureNoHTMLBlocks =             0x0020,
    FeatureNoHTMLSpans =              0x0040,
    FeatureTables =                   0x0100,
    FeatureStrikeThrough =            0x0200,
    FeaturePermissiveWWWAutoLinks =   0x0400,
    FeatureTasklists =                0x0800,
    FeatureUnderline =                0x4000,
    DialectGitHub =                   MarkdownDialectGitHub
};


enum class EnumFoo { // OK
    A,
    B = A,
    C,
    D,
    E,
    F
};

enum class EnumFoo2 { // OK
    A = 0,
    B = 1,
    C = 2,
    D = 4,
    E = (C | D)
};
