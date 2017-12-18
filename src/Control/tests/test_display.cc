#include <catch.hpp>
#include <so_5/all.hpp>
#include "Display.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace chrono;
// --------------------------------------------------------------------------
static int to_millisec( const std::chrono::steady_clock::duration& m )
{
    using namespace chrono;
    return duration_cast<milliseconds>(m).count();
}
// --------------------------------------------------------------------------
TEST_CASE("display calc_refresh_time", "[display][calc_refresh_time]" )
{
    using namespace chrono;
    // Display::calc_refresh_time( const double& speed, const size_t& height );
    // speed - м/сек, height - метры

    // базовая величина: при скорости 1 м/c и высоте 2 метра --> 1 сек
    CHECK( to_millisec(Display::calc_refresh_time(1,2)) == 1000 );

    // проверяем остальное
    CHECK( to_millisec(Display::calc_refresh_time(10,2)) == 100 );
    CHECK( to_millisec(Display::calc_refresh_time(0.1,2)) == 10000 );
    CHECK( to_millisec(Display::calc_refresh_time(1,1)) == 500 );

    // крайние значения (проверка исключения)
    REQUIRE_THROWS_AS( Display::calc_refresh_time(21,2), std::runtime_error );
    REQUIRE_THROWS_AS( Display::calc_refresh_time(0.001,2), std::runtime_error );
}
// --------------------------------------------------------------------------
