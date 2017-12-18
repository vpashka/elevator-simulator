#include "Elevator.h"
#include "TestControlAgent.h"
// --------------------------------------------------------------------------
using namespace std;
// --------------------------------------------------------------------------
TestControlAgent::TestControlAgent(context_t ctx
                 , so_5::mbox_t _control_mbox
                 , so_5::mbox_t _status_mbox
                 , const size_t _height )
 : so_5::agent_t{ ctx }
 , control_mbox(std::move(_control_mbox))
 , status_mbox(std::move(_status_mbox))
 , height(_height)
 , doors(Doors::State::dClosed)
{

}
// --------------------------------------------------------------------------
TestControlAgent::~TestControlAgent()
{
}
// --------------------------------------------------------------------------
void TestControlAgent::so_evt_start()
{
    // для начальной инициализации надо выставить текущее состояние
    double position = Elevator::calc_level_number(current_level, height);
    so_5::send< Elevator::msg_state_t >( status_mbox
                                        , current_level
                                        , position
                                        , Elevator::State::elStopped);
}
// --------------------------------------------------------------------------
void TestControlAgent::so_define_agent()
{
    // т.к. эти обработчики простые, напишем реализацию в виде лямбд
    so_subscribe_self().event( [this]( const msg_get_report_t& m ){
         return report;
    });


    // обработка команд работы с дверями
    so_subscribe(control_mbox)
      .event( [this]( const Doors::msg_open_t& m ){

        report.doors_open_cmd++;
        // и сразу имитируем что открылись
        report.doors.state  = Doors::State::dOpened;
        so_5::send< Doors::msg_state_t >(status_mbox, report.doors.state);
    })
     .event( [this]( const Doors::msg_close_t& m ){

        report.doors_close_cmd++;
        // и сразу имитируем что закрылись
        report.doors.state  = Doors::State::dClosed;
        so_5::send< Doors::msg_state_t >(status_mbox, report.doors.state);
    });

    // обработка команд для лифта
    so_subscribe(control_mbox)
      .event( [this]( const Elevator::msg_move_to_level_t& m ){
        report.elevator_move_cmd++;
        report.elevator_last_cmd = m;

        // если и так здесь, ничего не делаем
        if( current_level == m.level )
            return;

        double position = Elevator::calc_level_number(current_level, height);
        Elevator::State state = ( m.level < current_level ) ? Elevator::State::elMoveDown : Elevator::State::elMoveUp;

        so_5::send< Elevator::msg_state_t >(status_mbox, current_level, position, state);

        // через некоторое время имтируем что приехали
        so_5::send_delayed<msg_elevator_cmd_t>(*this, chrono::seconds(1), m );
    });

    so_subscribe_self().event( [this]( const msg_elevator_cmd_t& m ){

        current_level = m.cmd.level;
        report.level = current_level;
        double position = Elevator::calc_level_number(current_level, height);

        // посылаем состояние, что "приехали".
        so_5::send< Elevator::msg_state_t >( status_mbox
                                            , current_level
                                            , position
                                            , Elevator::State::elStopped);
    });
}
// --------------------------------------------------------------------------
