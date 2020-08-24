# qt6-qhash-signature

Warns and corrects the signature for qHash.
uint qHash(MyType x, uint seed) is replaced with size_t qHash(MyType x, size_t seed)
Also corrects the signature for qHashBits, qHashRange and qHashRangeCommutative.

This fix-it is intended to aid the user porting from Qt5 to Qt6.
Run this check with Qt5. The produced fixed code will compile on Qt6.