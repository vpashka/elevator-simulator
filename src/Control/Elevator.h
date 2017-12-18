#ifndef Elevator_H_
#define Elevator_H_
// --------------------------------------------------------------------------
#include <string>
#include <chrono>
#include <so_5/all.hpp>
// --------------------------------------------------------------------------
/*! Управление двигателем лифта
 * - движение вниз или вверх по команде управления
 * В реальности здесь должны были бы замыкаться какие-нибудь выходные реле,
 * но здесь просто имитируем изменение.
 *
 * \warning Этажи считаем с "1"
*/
class Elevator final:
	public so_5::agent_t
{
	public:
		Elevator( context_t ctx, const std::string& name
				  , so_5::mbox_t control_mbox
				  , so_5::mbox_t status_mbox
				  , size_t _nlevels // количество этажей
				  , double _height  // высота одного этажа
				  , double speed    // скорость лифта
				);

		virtual ~Elevator();

		/*! возможные состояния лифта */
		enum class State
		{
			elStopped, /*!< остановлен */
			elMoveUp,  /*!< движение вверх */
			elMoveDown /*!< движение вниз */
			// Авариийные состояния в этом проекте не рассматриваем
		};

		/*! команда на движение к заданому этажу */
		struct msg_move_to_level_t:
			public so_5::message_t
		{
			size_t level;
			msg_move_to_level_t( const size_t _level )
				: level(_level)
			{}
		};

		/*! проверка текущей позиции */
		struct msg_check_position_t:
			public so_5::message_t {};

		/*! текущее состояние лифта */
		struct msg_state_t:
			public so_5::message_t
		{
			size_t level;    /*!< текущий пройденный этаж */
			double position; /*!< текущее положение в метрах */
			State state;

			msg_state_t(  const size_t& _level
						  , const double& _pos
						  , const State& _state )
				: level(_level)
				, position(_pos)
				, state(_state)
			{}
		};

		struct msg_get_state_t:
			public so_5::message_t
		{};


		/*! вычисление перемещения при движении с заданной скоростью
		 * \param speed - скорость, м/c
		 * \param step_time - время движения
		 */
		static double calc_position_step( const double& speed,
										  const std::chrono::steady_clock::duration& step_time );

		/*! вычисление номера текущего этажа
		 * \param position - текущая позиция лифта, м
		 * \param height - высота этажа, м
		 */
		static size_t calc_level_number( const double& position, const double& height );

		/*! вычисление шага таймера для контроля позиции с точностью не более сантиметра
		 * в зависимости от заданной скорости.
		 * \param speed - скорость лифта, м/с. Разрешённый диапазон [ 0.1 ... 20 ]
		 * В случае ошибки генерирует исключение std::runtime_error
		 */
		static std::chrono::steady_clock::duration calc_step_time( const double& speed );

	protected:

		virtual void so_define_agent() override;
		virtual void so_evt_start() override;

		msg_state_t get_state( const msg_get_state_t& m );
		void cmd_move_to( const msg_move_to_level_t& m );
		void step( const msg_check_position_t& m );
		void go_up();
		void go_down();

	private:
		const std::string myname;
		so_5::mbox_t control_mbox;
		so_5::mbox_t status_mbox;
		const size_t nlevels; /*!< количество этажей */
		const double height;  /*!< высота этажа, м */
		const double maxposition; /*!< максимальное положение, м */
		const double speed;       /*!< скорость перемещения, м/с */
		size_t level = { 1 };     /*!< текущий этаж */
		double position = { 0 };  /*!< текущее положение, м */
		so_5::timer_id_t step_timer;

		/*! Шаг таймера, для контроля и управления текущей позицией
		 * шаг рассчитывается в зависимости от заданной скорости (см. calc_step_time())
		 */
		const std::chrono::steady_clock::duration step_time;

		State state = { State::elStopped };
		double target = { 0 };         /*!< целевая метка к которой едем, м */
		const double precision = 0.02; /*!< точность, м */
};
// --------------------------------------------------------------------------
namespace std
{
	std::string to_string( const Elevator::State& s );
}
// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------
