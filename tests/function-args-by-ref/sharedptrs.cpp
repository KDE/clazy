#include <QtCore/QSharedPointer>

struct Trivial {};

void test(QSharedPointer<int>) {}
void test(const QSharedPointer<int> &) {}
void test(const Trivial &) {}
