#include "Elevator.h"
#include "Doors.h"
// --------------------------------------------------------------------------
using namespace std;
// --------------------------------------------------------------------------
Doors::Doors(context_t ctx
			 , const std::string& name
			 , so_5::mbox_t _control_mbox
			 , so_5::mbox_t _status_mbox
			 , std::chrono::steady_clock::duration _door_time )
	: so_5::agent_t{ ctx }
	, myname(name)
	, control_mbox(std::move(_control_mbox))
	, status_mbox(std::move(_status_mbox))
	, doors_time(_door_time)
{

}
// --------------------------------------------------------------------------
Doors::~Doors()
{
}
// --------------------------------------------------------------------------
void Doors::so_define_agent()
{
	// при переходе в устойчивые состояния просто посылаем сообщение о статусе
	st_closed.on_enter([this] { so_5::send< msg_state_t >(status_mbox, State::dClosed); });
	st_opened.on_enter([this] { so_5::send< msg_state_t >(status_mbox, State::dOpened); });

	// переходные состояния
	// запускаем таймер и когда наступает время переходим в соответственющее устойчивое состояние
	// (в реальности тут будет подаваться команда на какое-то исполнительное реле)
	st_opening.on_enter([this]
	{
		so_5::send<msg_state_t>(status_mbox, State::dOpening);
		doors_open();
	}).just_switch_to<msg_finish_t>(st_opened);

	st_closing.on_enter([this]
	{
		so_5::send<msg_state_t>(status_mbox, State::dClosing);
		doors_close();
	}).just_switch_to<msg_finish_t>(st_closed);

	// запрос текущего состояния разрешён в любых состояниях
	so_subscribe_self()
	.in(st_opened)
	.in(st_closed)
	.in(st_closing)
	.in(st_opening)
	.event( &Doors::get_state );

	// команда закрыть, доступна только когда двери полностью открылись
	so_subscribe(control_mbox).in(st_opened)
	.just_switch_to<msg_close_t>(st_closing);

	// команда "открыть" доступна в режиме закрытых или закрывающихся дверей
	so_subscribe(control_mbox).in(st_closed).in(st_closing)
	.just_switch_to<msg_open_t>(st_opening);
}
// --------------------------------------------------------------------------
void Doors::so_evt_start()
{
	st_closed.activate();
}
// --------------------------------------------------------------------------
void Doors::doors_open()
{
	// тут должна быть работа с железом
	// но в нашем случае имитируется таймером
	so_5::send_delayed<msg_finish_t>(*this, doors_time);
}
// --------------------------------------------------------------------------
void Doors::doors_close()
{
	// тут должна быть работа с железом
	// но в нашем случае имитируется таймером
	so_5::send_delayed<msg_finish_t>(*this, doors_time);
}
// --------------------------------------------------------------------------
Doors::msg_state_t Doors::get_state( const msg_get_state_t& m )
{
	if( so_current_state() == st_opened )
		return msg_state_t(State::dOpened);

	if( so_current_state() == st_closed )
		return msg_state_t(State::dClosed);

	if( so_current_state() == st_opening )
		return msg_state_t(State::dOpening);

	if( so_current_state() == st_closing )
		return msg_state_t(State::dClosing);

	throw std::runtime_error(myname + "(get_state): Unknown state");
}
// --------------------------------------------------------------------------
namespace std
{
	std::string to_string( const Doors::State& s )
	{
		if( s == Doors::State::dOpened )
			return "Opened";

		if( s == Doors::State::dClosed )
			return "Closed";

		if( s == Doors::State::dOpening )
			return "Opening";

		if( s == Doors::State::dClosing )
			return "Closing";

		return "";
	}
}
// --------------------------------------------------------------------------
