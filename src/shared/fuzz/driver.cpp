//
// Created by Nicholas Robison on 8/23/20.
//

#include <iostream>
#include <string>

extern int LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size);

int main() {
#ifdef __AFL_HAVE_MANUAL_CONTROL
    while (__AFL_LOOP(1000)) {
#endif
    std::string line;

    std::getline(std::cin, line);

    LLVMFuzzerTestOneInput((std::uint8_t  *) line.data(), line.size());


#ifdef __AFL_HAVE_MANUAL_CONTROL
    }
#endif
}