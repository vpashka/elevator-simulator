#ifndef Control_H_
#define Control_H_
// --------------------------------------------------------------------------
#include <string>
#include <list>
#include <so_5/all.hpp>
#include "CPanel.h"
#include "Elevator.h"
#include "Doors.h"
// --------------------------------------------------------------------------
/*! Реализация управления
 * - синхронизация взаимодействия между элементами системы.
 * - формирование очереди команд и их отработка
 *
 * Возможные состояния:
 * - ожидание вызова (standby)
 * - погрузка/выгрузка (processing)
 * - движение к заданному этажу (moving)
 *
 * У "processing" есть три "вложенных" состояния(substate)
 * - открытие дверей (opening)
 * - ожидание c открытыми дверями (waiting)
 * - закрытие дверей (closing)
 *
 * \note Режим аварийной остановки и т.п. в этом демо-проекте не рассматриваем.
 * \note Реализуем вариант, когда в режиме standby лифт стоит с закрытими дверями
 *
 * Переходы:
 * standby --> processing --> moving --> [ standby ] или [ processing ]
 *
 * Переходы внутри processing:
 * [ opening --> waiting --> closing ]
 *
 * \warning Для простоты демо-проекта, не рассматриваются нештатные ситуации типа
 * "двери не открылись", "лифт недоехал" и т.п.
 * \warning Так же не рассматриваются датчики "движения", просто делается пауза на загрузку/выгрузку (wait_processing_time)
 *
*/
class Control final:
	public so_5::agent_t
{
	public:
		Control( context_t ctx
				 , const std::string& name
				 , so_5::mbox_t control_mbox
				 , so_5::mbox_t status_mbox
				 , std::chrono::steady_clock::duration doors_time // время открытия/закрытия дверей
				 , std::chrono::steady_clock::duration waiting_time  // время ожидания погрузки/выгрузки
			   );

		virtual ~Control();

		// состояния
		state_t st_standby{ this, "standby" };
		state_t st_processing{ this, "processing" };
		state_t st_moving{ this, "moving" };

		// substates
		state_t st_opening{ initial_substate_of{ st_processing }, "opening" };
		state_t st_waiting{ substate_of{ st_processing }, "waiting" };
		state_t st_closing{ substate_of{ st_processing }, "closing" };

		enum class State
		{
			stStandBy,
			stProcessing,
			stMoving
		};

		struct msg_delayed_action_t:
			public so_5::message_t
		{};

		struct msg_state_t:
			public so_5::message_t
		{
			State state;
			msg_state_t( State s )
				: state(s)
			{}
		};

		/*! функция планирования целей
		 * пересортировывает список целей в зависимости от направления движения
		 * \param targets - текущий список целей, который надо перепланировать
		 * \param state - текущее состояния лифта
		 * \param current_level - текущий этаж где находится лифт
		 */
		static void sort_targets( std::list<Button>& targets, Elevator::State state, size_t current_level );

	protected:

		virtual void so_define_agent() override;
		virtual void so_evt_start() override;

		void save_command( const CPanel::msg_press_button_t& m );
		void do_command();
		void remove_targets( size_t level );

	private:
		const std::string myname;
		so_5::mbox_t control_mbox;
		so_5::mbox_t status_mbox;
		std::list<Button> targets; /*!< цели (куда ехать) */
		Elevator::msg_state_t elevator; /*!< текущее состояние "лифта" */
		const std::chrono::steady_clock::duration doors_time;   /*!< время открытия/закрытия дверей */
		const std::chrono::steady_clock::duration waiting_time; /*!< время ожидания погрузки/выгрузки */
};
// --------------------------------------------------------------------------
namespace std
{
	std::string to_string( const Control::State& s );
}
// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------
