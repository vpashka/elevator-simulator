#ifndef Doors_H_
#define Doors_H_
// --------------------------------------------------------------------------
#include <string>
#include <so_5/all.hpp>
#include "Elevator.h"
// --------------------------------------------------------------------------
/*! Управление дверями.
 * Двери имеют два устойчивых состояния:
 * - открыты (opened)
 * - закрыты (closed)
 * И два переходных состояния:
 * - открываются (opening)
 * - закрываются (closing)
 *
 * Переходы:
 * opened --> closing --> closed
 * closed --> opening --> opened
 *
 * Управление приходит от Control
*/
class Doors final:
	public so_5::agent_t
{
	public:
		Doors( context_t ctx
			   , const std::string& name
			   , so_5::mbox_t control_mbox
			   , so_5::mbox_t status_mbox
			   , std::chrono::steady_clock::duration doors_time );

		virtual ~Doors();

		// возможные состояния дверей
		state_t st_opening{ this, "opening" };
		state_t st_closing{ this, "closing" };
		state_t st_closed{ this, "closed" };
		state_t st_opened{ this, "opened" };

		enum class State
		{
			dOpened,
			dClosed,
			dOpening,
			dClosing,
		};

		// команды
		struct msg_open_t:
			public so_5::message_t {};

		struct msg_close_t:
			public so_5::message_t {};

		struct msg_finish_t:
			public so_5::message_t {};

		/*! текущий статус */
		struct msg_state_t:
			public so_5::message_t
		{
			State state;
			msg_state_t( State s )
				: state(s)
			{}
		};

		struct msg_get_state_t:
			public so_5::message_t
		{};

	protected:

		virtual void so_define_agent() override;
		virtual void so_evt_start() override;

		msg_state_t get_state( const msg_get_state_t& m );

		// функции выполняющие реальную работу "с железом"
		// (подача команд на исполнительные устройства)
		void doors_open();
		void doors_close();

	private:
		const std::string myname;
		so_5::mbox_t control_mbox;
		so_5::mbox_t status_mbox;

		/*! время открывания/закрывания дверей */
		const std::chrono::steady_clock::duration doors_time;
};
// --------------------------------------------------------------------------
namespace std
{
	std::string to_string( const Doors::State& s );
}
// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------
