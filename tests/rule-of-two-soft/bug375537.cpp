#include <thread>

void test()
{
    std::thread t;
    t = std::thread(); // OK
}
