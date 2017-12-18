#include <catch.hpp>
#include <so_5/all.hpp>
#include "Doors.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace chrono;
// --------------------------------------------------------------------------
extern so_5::wrapped_env_t global_env;
static so_5::mbox_t agent_mbox;

// время открытия/закрытия дверей для теста делаем пошустрее
static steady_clock::duration doors_time = milliseconds(100);

static const std::string command_mbox_name = "test_doors_command_mbox";
static const std::string status_mbox_name = "test_doors_status_mbox";
// --------------------------------------------------------------------------
static void InitTest( so_5::environment_t& env )
{
    if( agent_mbox )
        return;

    // ящики необходимые для работы Doors
    auto pub_status_mbox = env.create_mbox(status_mbox_name);
    auto pub_command_mbox = env.create_mbox(command_mbox_name);

    env.introduce_coop(so_5::autoname,
                       so_5::disp::one_thread::create_private_disp( env )->binder(),
                       [&]( so_5::coop_t& coop )
    {
        auto agent = coop.make_agent<Doors >("doors", pub_command_mbox, pub_status_mbox, doors_time);
        agent_mbox = agent->so_direct_mbox();
    });

    // пауза чтобы агенты запустились
    std::this_thread::sleep_for(seconds(1));
}
// --------------------------------------------------------------------------
static Doors::State get_doors_state()
{
    auto result = so_5::request_value<Doors::msg_state_t, Doors::msg_get_state_t>(
                agent_mbox, so_5::infinite_wait
    );

    return result.state;
}
// --------------------------------------------------------------------------
template< typename cmd_t >
static void send_command()
{
    auto mbox = global_env.environment().create_mbox(command_mbox_name);
    so_5::send<cmd_t>(mbox);
}
// --------------------------------------------------------------------------
static void processing_command_pause()
{
    std::this_thread::sleep_for(doors_time + milliseconds(50));
}
// --------------------------------------------------------------------------
TEST_CASE("open doors", "[doors][open]" )
{
    InitTest(global_env.environment());

    REQUIRE( agent_mbox );

    send_command<Doors::msg_open_t>();

    processing_command_pause();

    CHECK( get_doors_state() == Doors::State::dOpened );
}
// --------------------------------------------------------------------------
TEST_CASE("close doors", "[doors][close]" )
{
    InitTest(global_env.environment());

    REQUIRE( agent_mbox );

    send_command<Doors::msg_close_t>();

    processing_command_pause();

    CHECK( get_doors_state() == Doors::State::dClosed );
}
// --------------------------------------------------------------------------
TEST_CASE("open -> close -> open", "[doors][cycle]" )
{
    InitTest(global_env.environment());

    REQUIRE( agent_mbox );

    send_command<Doors::msg_open_t>();
    processing_command_pause();
    REQUIRE( get_doors_state() == Doors::State::dOpened );

    send_command<Doors::msg_close_t>();
    processing_command_pause();
    REQUIRE( get_doors_state() == Doors::State::dClosed );

    send_command<Doors::msg_open_t>();
    processing_command_pause();
    REQUIRE( get_doors_state() == Doors::State::dOpened );

}
// --------------------------------------------------------------------------
