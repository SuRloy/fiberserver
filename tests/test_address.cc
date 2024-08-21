#include "address.h"
#include <iostream>

using namespace zy;

void test_ipv4() {
    auto addr = IPv4Address::Create("192.168.137.1", 8080);
    if (!addr) {
        std::cout << "ipv4 address create failed!" << std::endl;
    }
    std::cout << addr->toString() << std::endl;
}

int main(int argc, char *argv[]) {
    test_ipv4();
    return 0;
}