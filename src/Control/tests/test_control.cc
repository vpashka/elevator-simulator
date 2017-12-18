#include <catch.hpp>
#include <chrono>
#include "Control.h"
#include "TestControlAgent.h"
// --------------------------------------------------------------------------
using namespace std;
using namespace chrono;
// --------------------------------------------------------------------------
extern so_5::wrapped_env_t global_env;
static so_5::mbox_t test_agent_mbox;
// --------------------------------------------------------------------------
// для тестирования все времена делаем "маленькими", чтобы тесты пошустрее проходили
static steady_clock::duration doors_processing_time = milliseconds(100);
static steady_clock::duration waiting_processing_time = milliseconds(300);
static const uint height = 1;
static const std::string command_mbox_name = "test_control_command_mbox";
static const std::string status_mbox_name = "test_control_status_mbox";
// --------------------------------------------------------------------------
static void InitTest( so_5::environment_t& env )
{
    if( test_agent_mbox )
        return;

    // ящики необходимые для работы Control
    auto pub_status_mbox = env.create_mbox(status_mbox_name);
    auto pub_command_mbox = env.create_mbox(command_mbox_name);

    env.introduce_coop(so_5::autoname,
                       // so_5::disp::active_obj::create_private_disp( env )->binder(),
                       so_5::disp::one_thread::create_private_disp( env )->binder(),
                       [&]( so_5::coop_t& coop )
    {
        coop.make_agent<Control>("control"
                                 , pub_command_mbox
                                 , pub_status_mbox
                                 , doors_processing_time
                                 , waiting_processing_time );

        auto test_agent = coop.make_agent<TestControlAgent>(
                                              pub_command_mbox
                                             , pub_status_mbox
                                             , height );

        test_agent_mbox = test_agent->so_direct_mbox();
    });

    // пауза чтобы агенты запустились
    std::this_thread::sleep_for(seconds(1));
}
// --------------------------------------------------------------------------
static TestControlAgent::msg_report_t get_report()
{
    auto result = so_5::request_value<TestControlAgent::msg_report_t, TestControlAgent::msg_get_report_t>(
                test_agent_mbox, so_5::infinite_wait
    );

    return result;
}
// --------------------------------------------------------------------------
template< typename cmd_t, typename... _Args >
static void send_command( _Args&& ... __args )
{
    auto mbox = global_env.environment().create_mbox(command_mbox_name);
    so_5::send<cmd_t>(mbox, std::forward<_Args>(__args)...);
}
// --------------------------------------------------------------------------
template< typename cmd_t, typename... _Args >
static void send_to_testagent( _Args&& ... __args )
{
    so_5::send<cmd_t>(test_agent_mbox, std::forward<_Args>(__args)...);
}
// --------------------------------------------------------------------------
TEST_CASE("default control", "[control][default]" )
{
    InitTest(global_env.environment());

    REQUIRE( test_agent_mbox );

    // проверим начальные условия
    auto report = get_report();
    REQUIRE( report.doors_open_cmd == 0 );
    REQUIRE( report.doors_close_cmd == 0 );
    REQUIRE( report.elevator_move_cmd == 0 );

    // посылаем команду "этаж 1"
    // начальные условия - лифт внизу
    send_command<CPanel::msg_press_button_t>( Button('1', 1, Button::btnExternal) );

    // пауза на отработку команды
    std::this_thread::sleep_for(milliseconds(150));

    // проверяем, что была подана команда на открытие дверей
    report = get_report();
    REQUIRE( report.doors_open_cmd == 1 );
    REQUIRE( report.doors_close_cmd == 0 );

    // дальше идёт ожидание
    std::this_thread::sleep_for(waiting_processing_time + chrono::milliseconds(50));

    // проверяем, что была подана команда на закрытие
    report = get_report();
    REQUIRE( report.doors_close_cmd == 1 );
}
// --------------------------------------------------------------------------
TEST_CASE("sort_targets", "[control][sort_targets]" )
{
    std::list<Button> tlist = {
        { '4', 4, Button::btnExternal },
        { '1', 1, Button::btnExternal },
        { '5', 5, Button::btnExternal },
        { 'a', 10, Button::btnExternal },
        { '6', 6, Button::btnExternal }
    };

    // если мы стоим то сортировка под последнюю цель в списке
    // т.е. считается что список наполняется по мере прихода команд
    // в данном случае если мы стоим на 5, а первые в списке 4,1
    // то должна выстроиться цепочка 5 --> 4 1
    auto tlist1(tlist);

    Control::sort_targets(tlist1, Elevator::State::elStopped, 5);

    CHECK( tlist1.begin()->level == 5 );
    CHECK( (++tlist1.begin())->level == 4 );

    // если мы едем "вверх", то по возрастанию начинаю от текущего
    auto tlist2(tlist);
    Control::sort_targets(tlist2, Elevator::State::elMoveUp, 5);
    CHECK( tlist2.begin()->level == 5 );
    CHECK( (++tlist2.begin())->level == 6 );

    // если мы едем "вниз", то по убыванию начиная от текущего
    auto tlist3(tlist);
    Control::sort_targets(tlist3, Elevator::State::elMoveDown, 5);
    CHECK( tlist3.begin()->level == 5 );
    CHECK( (++tlist3.begin())->level == 4 );
}
// --------------------------------------------------------------------------
TEST_CASE("processing", "[control][processing]" )
{
    InitTest(global_env.environment());

    REQUIRE( test_agent_mbox );

    // проверим начальные условия
    auto report = get_report();
    REQUIRE( report.level == 1 );

    // отработка последовательности команд
    send_command<CPanel::msg_press_button_t>( Button('1', 5, Button::btnExternal) );
    send_command<CPanel::msg_press_button_t>( Button('1', 4, Button::btnExternal) );
    send_command<CPanel::msg_press_button_t>( Button('1', 2, Button::btnExternal) );

    // пауза на отработку
    std::this_thread::sleep_for(seconds(5));

    report = get_report();
    REQUIRE( report.level == 5 );

    send_command<CPanel::msg_press_button_t>( Button('1', 1, Button::btnExternal) );
    send_command<CPanel::msg_press_button_t>( Button('1', 2, Button::btnExternal) );
    send_command<CPanel::msg_press_button_t>( Button('1', 3, Button::btnExternal) );

    // пауза на отработку
    std::this_thread::sleep_for(seconds(5));

    report = get_report();
    REQUIRE( report.level == 1 );
}
// --------------------------------------------------------------------------
