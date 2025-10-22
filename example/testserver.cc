#include <iostream>

bool isLittleEndian() {
    uint32_t test = 0x12345678;
    uint8_t* bytes = (uint8_t*)&test;
    return bytes[0] == 0x78;  // 小端系统第一个字节是0x78
}

int main() {
    if (isLittleEndian()) {
        std::cout << "系统使用小端字节序!" << std::endl;
    }
    else {
        std::cout << "系统使用大端字节序!" << std::endl;
    }
    return 0;
}