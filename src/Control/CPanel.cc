#include <sstream>
#include <cstdio>
#include <limits>
#include <unistd.h>
#include <fcntl.h>
#include <error.h>
#include "Display.h"
#include "CPanel.h"
// --------------------------------------------------------------------------
using namespace std;
// --------------------------------------------------------------------------
CPanel::CPanel(context_t _ctx
			   , const std::string& _name
			   , so_5::mbox_t _control_mbox
			   , so_5::mbox_t _status_mbox
			   , const std::vector<Button>& _in_buttons
			   , const std::vector<Button>& _ext_buttons )
	: so_5::agent_t{ _ctx }
	, myname(_name)
	, control_mbox(std::move(_control_mbox))
	, status_mbox(std::move(_status_mbox))
	, in_buttons(_in_buttons)
	, ext_buttons(_ext_buttons)
{
}
// --------------------------------------------------------------------------
CPanel::~CPanel()
{
}
// --------------------------------------------------------------------------
void CPanel::so_define_agent()
{
	so_subscribe_self().event( &CPanel::on_read_input );
}
// --------------------------------------------------------------------------
void CPanel::so_evt_start()
{
	oldflags = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, oldflags | O_NONBLOCK);

	readinput_timer = so_5::send_periodic< msg_read_input_t >(
						  *this,
						  std::chrono::seconds::zero(),
						  std::chrono::milliseconds(100) );
}
// --------------------------------------------------------------------------
void CPanel::so_evt_finish()
{
	fcntl(STDIN_FILENO, F_SETFL, oldflags);
}
// --------------------------------------------------------------------------
void CPanel::on_read_input( const msg_read_input_t& m )
{
	// ожидание надо прерывать
	// чтобы "мочь" завершать работу
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);

	// будем ждать select_timeout_sec
	struct timeval tv = {select_timeout_sec, 0};
	ssize_t ret = select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);

	if( ret < 0 )
	{
		ostringstream err;
		err << " read input error: ("
			<< "(" << errno << ")"
			<< strerror(errno);

		// отображаем ошибку на экране..
		do_send_message(err.str()); //      throw std::runtime_error(err.str());
		return;
	}

	if( !FD_ISSET(STDIN_FILENO, &fds) )
		return;

	// т.к. у нас команды односимвольные
	// а фактически в терминале можно набирать любое количество
	// пока не нажат enter
	// то для простоты отрабатываем только "последний введёный символ"
	int bcode = std::numeric_limits<int>::max();

	while( true )
	{
		char c;
		ret = ::read( STDIN_FILENO, &c, sizeof(c) );

		if( ret < 0 )
		{
			if( errno != EAGAIN )
			{
				ostringstream err;
				err << " read input error: ("
					<< "(" << errno << ")"
					<< strerror(errno);
				do_send_message(err.str()); // throw std::runtime_error(err.str());
			}

			break;
		}

		if( ret == 0 )
			break;

		if( c != '\n' )
			bcode = c;
	}

	if( bcode != std::numeric_limits<int>::max() )
	{
		auto btn = get_button_by_code(bcode);

		if( btn.type != Button::btnUnknown )
			do_send_command(btn);
	}
}
// --------------------------------------------------------------------------
void CPanel::do_send_message( const std::string& txt )
{
	so_5::send< Display::msg_display_message >(control_mbox, txt);
}
// --------------------------------------------------------------------------
void CPanel::do_send_command( const Button& btn )
{
	so_5::send< msg_press_button_t >(control_mbox, btn);
}
// --------------------------------------------------------------------------
Button CPanel::get_button_by_code( const char key ) const
{
	auto it = std::find_if( in_buttons.begin(), in_buttons.end(), [&key](const Button & btn)
	{
		return (key == btn.symbol);
	});

	if( it != in_buttons.end() )
		return (*it);

	auto it2 = std::find_if( ext_buttons.begin(), ext_buttons.end(), [&key](const Button & btn)
	{
		return (key == btn.symbol);
	});

	if( it2 != ext_buttons.end() )
		return (*it2);

	return Button('\0', 0, Button::btnUnknown);
}
// --------------------------------------------------------------------------


