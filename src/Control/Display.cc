#include <iostream>
#include <iomanip>
#include "Display.h"
#include "Button.h"
// --------------------------------------------------------------------------
using namespace std;
// --------------------------------------------------------------------------
// можно было бы сделать цвета настраиваемыми (для простоты не будем)
// \warning коды актуальные только для bash

// подсветка по умолчанию (не выбранного)
static const string fg_normal = "\033[0;37m"; // gray
static const string bg_normal = "\033[40m"; // black

// подсветка текущего этажа где находится лифт
static const string fg_current = "\033[37m"; // white
static const string bg_current = "\033[43m"; // yellow(orange)

// подсветка выбранного этажа (целевые этажи)
static const string fg_select = "\033[34m"; // blue
static const string bg_select = "\033[40m"; // black
// --------------------------------------------------------------------------
Display::Display( context_t _ctx
				  , const std::string& _name
				  , so_5::mbox_t _control_mbox
				  , so_5::mbox_t _status_mbox
				  , const std::vector<Button>& _in_buttons
				  , const std::vector<Button>& _ext_buttons
				  , const size_t _levels
				  , const double _height
				  , const double _speed )
	: so_5::agent_t{ _ctx }
	, myname(_name)
	, control_mbox(std::move(_control_mbox))
	, status_mbox(std::move(_status_mbox))
	, int_buttons(_in_buttons)
	, ext_buttons(_ext_buttons)
	, selected_floors(_levels + 1, false)
	, selected_ext_buttons(_levels + 1, false)
	, selected_int_buttons(_levels + 1, false)
	, elevator(0, 0, Elevator::State::elStopped)
	, doors(Doors::State::dClosed)
	, control(Control::State::stStandBy)
	, nlevels(_levels)
	, height(_height)
	, speed(_speed)
	, display_refresh_time(Display::calc_refresh_time(_speed, _height))
{
}
// --------------------------------------------------------------------------
Display::~Display()
{
}
// --------------------------------------------------------------------------
std::chrono::steady_clock::duration Display::calc_refresh_time( const double& speed, const double& height )
{
	// Базовая частота: При скорости 1 м/с и высоте этажа 2м --> 1 сек.
	// т.е на всякий берём 2 точки на единицу скорости
	// чем выше скорость тем чаще обновляем
	if( speed > 20.0 )
	{
		ostringstream err;
		err << "(Display::calc_refresh_time): BAD speed= " << speed << " Must be < 20.0";
		throw std::runtime_error(err.str());
	}

	if( speed < 0.1 )
	{
		ostringstream err;
		err << "(Display::calc_refresh_time): BAD speed= " << speed << " Must be > 0.1";
		throw std::runtime_error(err.str());
	}

	// микросекунд должно хватать, поэтому считаем в микросекундах
	// 1e6 <-- это перевод сек в мкс

	double refresh_time_microsec = (height / 2.0) * 1e6 / speed;
	return chrono::microseconds(lroundf(refresh_time_microsec));
}
// --------------------------------------------------------------------------
void Display::so_define_agent()
{
	so_subscribe(status_mbox).event( &Display::on_elevator_event );
	so_subscribe(status_mbox).event( &Display::on_doors_event );
	so_subscribe(status_mbox).event( &Display::on_control_event );
	so_subscribe(control_mbox).event( &Display::on_cpanel_event );
	so_subscribe(control_mbox).event( &Display::on_display_message );
	so_subscribe_self().event( &Display::on_refresh );
	so_subscribe_self().event( &Display::on_update_text );
}
// --------------------------------------------------------------------------
void Display::so_evt_start()
{
	clear_screen();

	refresh_timer = so_5::send_periodic< msg_refresh_t >(
						*this,
						std::chrono::seconds::zero(),
						display_refresh_time );
}
// --------------------------------------------------------------------------
void Display::on_elevator_event( const Elevator::msg_state_t& m )
{
	// если мы приехали на этаж, можно снять метку "выбора"
	if( m.state == Elevator::State::elStopped )
	{
		selected_floors[m.level] = false;
		selected_ext_buttons[m.level] = false;
		selected_int_buttons[m.level] = false;
	}

	elevator = m;
}
// --------------------------------------------------------------------------
void Display::on_doors_event(const Doors::msg_state_t& m)
{
	if( doors.state != Doors::State::dOpened && m.state == Doors::State::dOpened )
		on_display_message(msg_display_message("Doors opened.."));
	else if( doors.state != Doors::State::dClosed && m.state == Doors::State::dClosed )
		on_display_message(msg_display_message("Doors closed.."));

	doors = m;
}
// --------------------------------------------------------------------------
void Display::on_cpanel_event( const CPanel::msg_press_button_t& m )
{
	// отмечаем выбранный этаж (если это не текущий)
	if( m.btn.level > 0
			&& m.btn.level <= selected_floors.size()
			&& m.btn.level != elevator.level )
	{
		selected_floors[m.btn.level] = true;

		if( m.btn.type == Button::btnExternal )
			selected_ext_buttons[m.btn.level] = true;
		else if( m.btn.type == Button::btnInternal )
			selected_int_buttons[m.btn.level] = true;
	}
}
// --------------------------------------------------------------------------
void Display::on_control_event(const Control::msg_state_t& m)
{
	control = m;
}
// --------------------------------------------------------------------------
void Display::on_display_message( const msg_display_message& m )
{
	//! \todo по хорошему здесь нужна ещё защита от переполнения..
	// для простоты опустим
	qmsg.push(m.text);

	if( !qmsg_timer.is_active() )
	{
		qmsg_timer = so_5::send_periodic< msg_update_text_t >(
						 *this,
						 std::chrono::seconds::zero(),
						 display_message_time );
	}
}
// --------------------------------------------------------------------------
void Display::on_refresh( const Display::msg_refresh_t& m )
{
	clear_screen();
	display_status();
	display_hseparator();
	display_level();
	display_hseparator();
	display_message();
	display_hseparator();
	display_buttons();
	display_hseparator();

	//    cout << "\033[0;40f"; /* Move cursor 40,0 */
	display_parameters();
}
// --------------------------------------------------------------------------
void Display::clear_screen()
{
	// Не будем утяжелять проект использованием ncurses
	// поэтому используем "магию" терминала linux

	cout << "\033[2J";   /* Clear */
	cout << "\033[0;0f"; /* Move cursor to 0,0 */
}
// --------------------------------------------------------------------------
void Display::display_status()
{
	cout << setw(17) << std::right << "ELEVATOR: " << to_string(elevator.state)
		 << endl
		 << setw(17) << std::right << "DOORS: " << to_string(doors.state)
		 << endl;
	//	cout << setw(17) << std::right << "POS: " << elevator.position
	//		 << endl;
}
// --------------------------------------------------------------------------
void Display::display_level()
{
	cout << setw(17) << std::right << "LEVEL: ";

	for( size_t i = 1; i <= nlevels; i++ )
	{
		if( i == elevator.level )
			cout << fg_current << bg_current;
		else if( selected_floors[i] )
			cout << fg_select << bg_select;

		cout << setw(2) << std::right << i
			 << fg_normal << bg_normal << " ";
	}

	cout << std::left << endl;
}
// --------------------------------------------------------------------------
void Display::display_message()
{
	cout << setw(17) << std::right << "MESSAGE: "
		 << text
		 << endl;
}
// --------------------------------------------------------------------------
void Display::display_hseparator()
{
	// расчитываем на стандартный терминал 80 символов
	cout << std::left << setfill('-') << setw(80) << "-" << setfill(' ') << endl;
}
// --------------------------------------------------------------------------
void Display::display_parameters()
{
	cout << "[ "
		 << "speed: " << speed << " m/s"
		 << "  level height: " << height << " m"
		 << " ]"
		 << endl;
}
// --------------------------------------------------------------------------
void Display::on_update_text(const Display::msg_update_text_t& m)
{
	// вынимаем очередное сообщение из очереди
	// оно отобразиться при очередном on_refresh().
	if( qmsg.empty() )
	{
		text = "";
		qmsg_timer.release();
		return;
	}

	text = qmsg.front();
	qmsg.pop();
}
// --------------------------------------------------------------------------
void Display::display_buttons()
{
	const size_t left_offset = 17;
	// выводим "клавиатуру"
	// сверху номер этажа, снизу название кнопки на клавиатуре
	//          SELECT: 1 2 3 4 5 6
	// ELEVATOR BUTTON: 1 2 3 4 5 6
	//    LEVEL BUTTON: a b c d e f

	cout << setw(left_offset) << std::right << "SELECT: ";

	for( size_t i = 1; i <= nlevels; i++ )
	{
		cout << setw(2) << std::right << i
			 << fg_normal << bg_normal << " ";
	}

	cout << endl;

	cout << setw(left_offset) << std::right << "ELEVATOR BUTTON: ";

	for( size_t i = 0; i < int_buttons.size(); i++ )
	{
		if( selected_int_buttons[i + 1] )
			cout << fg_select << bg_select;

		cout << setw(2) << std::right << int_buttons[i].symbol
			 << fg_normal << bg_normal << " ";
	}

	cout << endl;

	cout << setw(left_offset) << std::right << "LEVEL BUTTON: ";

	for( size_t i = 0; i < ext_buttons.size(); i++ )
	{
		if( selected_ext_buttons[i + 1] )
			cout << fg_select << bg_select;

		cout << setw(2) << std::right << ext_buttons[i].symbol
			 << fg_normal << bg_normal << " ";
	}

	cout << std::left << endl;


	const std::string help_text = "..press 'key' and 'ENTER'..";

	// рассчёт центрирования для текста (количество пробелов слева при выводе)
	// на каждый символ этажа выводиться 3 символа (две цифры + пробел)
	// а также учитываем левую часть для подписей
	size_t maxwidth = left_offset + nlevels * 3;
	int w =  ( maxwidth - help_text.size() ) / 2;

	if( w < 0 )
		w = 0;

	// вначале выводим нужное количество пробелов, потом текст
	cout << setw(w) << " " << help_text << endl;
}
// --------------------------------------------------------------------------


