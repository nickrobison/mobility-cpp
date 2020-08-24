//
// Created by Nicholas Robison on 8/23/20.
//

#include <shared/QuotedLineSplitter.hpp>
#include <string>

extern int LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size) {

    std::string input((const char *) data);

    const auto output = shared::QuotedStringSplitter(input);
    return 0;
}
