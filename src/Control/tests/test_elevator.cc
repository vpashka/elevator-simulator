#include <catch.hpp>
#include <chrono>
#include "Elevator.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace chrono;
// --------------------------------------------------------------------------
extern so_5::wrapped_env_t global_env;
static so_5::mbox_t agent_mbox;
// --------------------------------------------------------------------------
static const uint floors = 3;
static const uint height = 1;
static const double speed = 1.0; // м/c
static const std::string command_mbox_name = "test_elevator_command_mbox";
static const std::string status_mbox_name = "test_elevator_status_mbox";
// --------------------------------------------------------------------------
static void InitTest( so_5::environment_t& env )
{
    if( agent_mbox )
        return;

    // ящики необходимые для работы Elevator
    auto pub_status_mbox = env.create_mbox(status_mbox_name);
    auto pub_command_mbox = env.create_mbox(command_mbox_name);

    env.introduce_coop(so_5::autoname,
                       // so_5::disp::active_obj::create_private_disp( env )->binder(),
                       so_5::disp::one_thread::create_private_disp( env )->binder(),
                       [&]( so_5::coop_t& coop )
    {
        auto agent = coop.make_agent<Elevator>("elevator"
                                             , pub_command_mbox
                                             , pub_status_mbox
                                             , floors
                                             , height
                                             , speed );
        agent_mbox = agent->so_direct_mbox();
    });

    // пауза чтобы агенты запустились
    std::this_thread::sleep_for(seconds(1));
}
// --------------------------------------------------------------------------
static Elevator::State get_elevator_state()
{
    auto result = so_5::request_value<Elevator::msg_state_t, Elevator::msg_get_state_t>(
                agent_mbox, so_5::infinite_wait
    );

    return result.state;
}
// --------------------------------------------------------------------------
static uint get_elevator_level()
{
    auto result = so_5::request_value<Elevator::msg_state_t, Elevator::msg_get_state_t>(
                agent_mbox, so_5::infinite_wait
    );

    return result.level;
}
// --------------------------------------------------------------------------
template< typename cmd_t, typename... _Args >
static void send_command( _Args&& ... __args )
{
    auto mbox = global_env.environment().create_mbox(command_mbox_name);
    so_5::send<cmd_t>(mbox, std::forward<_Args>(__args)...);
}
// --------------------------------------------------------------------------
static int to_microsec( const std::chrono::steady_clock::duration& m )
{
    using namespace chrono;
    return duration_cast<microseconds>(m).count();
}
// --------------------------------------------------------------------------
TEST_CASE("elevator calc_step_time", "[elevator][calc_step_time]" )
{
    using namespace chrono;
    // Elevator::calc_calc_step_time( speed[м/с] ) == [ steady_clock::duration ];
    CHECK( to_microsec(Elevator::calc_step_time(0.1)) == 100000 );
    CHECK( to_microsec(Elevator::calc_step_time(1)) == 10000 );
    CHECK( to_microsec(Elevator::calc_step_time(10)) == 1000 );
    CHECK( to_microsec(Elevator::calc_step_time(20)) == 500 );

    // крайние значения (проверка исключения)
    REQUIRE_THROWS_AS( Elevator::calc_step_time(20.1), std::runtime_error );
    REQUIRE_THROWS_AS( Elevator::calc_step_time(0.05), std::runtime_error );
}
// --------------------------------------------------------------------------
TEST_CASE("elevator calc_position_step", "[elevator][calc_position_step]" )
{
    using namespace chrono;
    // Elevator::calc_position_step( speed[м/с], timer_step[сек] ) == [ метров ];
    CHECK( Elevator::calc_position_step(0, seconds(0)) == 0 );
    CHECK( Elevator::calc_position_step(1, seconds(1)) == 1 );
    CHECK( Elevator::calc_position_step(0.5, seconds(1)) == 0.5 );
    CHECK( Elevator::calc_position_step(5.0, milliseconds(1)) == 0.005 );
    CHECK( Elevator::calc_position_step(5.0, milliseconds(10)) == 0.05 );

    CHECK( Elevator::calc_position_step(1, milliseconds(100)) == 0.1 );
    CHECK( Elevator::calc_position_step(0.5, milliseconds(100)) == 0.05 );
}
// --------------------------------------------------------------------------
TEST_CASE("elevator calc_level_number", "[elevator][calc_level_number]" )
{
    // для высоты 1 м
    CHECK( Elevator::calc_level_number(0, 1) == 1 );
    CHECK( Elevator::calc_level_number(0.5, 1) == 2 );
    CHECK( Elevator::calc_level_number(1.0, 1) == 2 );
    CHECK( Elevator::calc_level_number(1.2, 1) == 2 );
    CHECK( Elevator::calc_level_number(1.6, 1) == 3 );
    CHECK( Elevator::calc_level_number(2.0, 1) == 3 );
    CHECK( Elevator::calc_level_number(2.5, 1) == 4 );

    CHECK( Elevator::calc_level_number(2.5, 2.5) == 2 );
    CHECK( Elevator::calc_level_number(5, 2.5) == 3 );
    CHECK( Elevator::calc_level_number(7.5, 2.5) == 4 );
    CHECK( Elevator::calc_level_number(10, 2.5) == 5 );
}
// --------------------------------------------------------------------------
TEST_CASE("move up", "[elevator][moveup]" )
{
    InitTest(global_env.environment());

    REQUIRE( agent_mbox );
    REQUIRE( floors >= 3 );

    // начальные условия - лифт внизу
    REQUIRE( get_elevator_level() == 1 );

    send_command<Elevator::msg_move_to_level_t>(2);

    std::this_thread::sleep_for(milliseconds(100));

    // проверяем, что начал движение
    CHECK( get_elevator_state() == Elevator::State::elMoveUp );

    std::this_thread::sleep_for(seconds(2));

    // проверяем, что приехал на заданный этаж
    REQUIRE( get_elevator_level() == 2 );

    // едем на следующий этаж
    send_command<Elevator::msg_move_to_level_t>(3);
    std::this_thread::sleep_for(seconds(3));
    // проверяем, что приехал на заданный этаж
    REQUIRE( get_elevator_level() == 3 );
}
// --------------------------------------------------------------------------
TEST_CASE("move down", "[elevator][movedown]" )
{
    InitTest(global_env.environment());

    REQUIRE( agent_mbox );
    REQUIRE( floors >= 3 );

    // начальные условия - лифт наверху
    REQUIRE( get_elevator_level() == 3 );

    send_command<Elevator::msg_move_to_level_t>(2);

    std::this_thread::sleep_for(milliseconds(100));

    // проверяем, что начал движение
    CHECK( get_elevator_state() == Elevator::State::elMoveDown );

    std::this_thread::sleep_for(seconds(2));

    // проверяем, что приехал на заданный этаж
    REQUIRE( get_elevator_level() == 2 );

    // едем на следующий этаж
    send_command<Elevator::msg_move_to_level_t>(1);
    std::this_thread::sleep_for(seconds(2));
    // проверяем, что приехал на первый этаж
    REQUIRE( get_elevator_level() == 1 );
}
// --------------------------------------------------------------------------
