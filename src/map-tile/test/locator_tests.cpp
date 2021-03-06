//
// Created by Nicholas Robison on 7/23/20.
//

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch2/catch.hpp"
#include "map-tile/coordinates/LocaleLocator.hpp"

using namespace std;
using namespace mt::coordinates;

TEST_CASE("Simple Locator Test", "[locator]") {

    // Let's create two tiles that cover a 5x5 grid
    const auto c1 = Coordinate2D(0, 0);
    const auto c2 = Coordinate2D(2, 5);

    const auto d1 = Coordinate2D(0, 3);
    const auto d2 = Coordinate2D(5, 5);

    const auto l = LocaleLocator({
                          LocaleLocator::value{mt_tile(c1, c2), 1},
                          LocaleLocator::value{mt_tile(d1, d2), 2},
                  });

    // Try in the center of one
    const auto loc1 = l.get_locale(Coordinate2D(1, 1));
    REQUIRE(loc1 == 1);

    const auto loc2 = l.get_locale(Coordinate2D(4, 4));
    REQUIRE(loc2 == 2);

    // Now, right on the edge
    const auto loc3 = l.get_locale(Coordinate2D(3, 5));
    REQUIRE(loc3 == 2);

    // Now, right on the edge
    const auto loc4 = l.get_locale(Coordinate2D(2, 5));
    REQUIRE(loc4 == 1);

    REQUIRE_THROWS_WITH( l.get_locale(Coordinate2D(7, 7)), "Out of bounds");
}