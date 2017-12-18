#ifndef Display_H_
#define Display_H_
// --------------------------------------------------------------------------
#include <string>
#include <queue>
#include <vector>
#include <so_5/all.hpp>
#include "Elevator.h"
#include "Doors.h"
#include "CPanel.h"
#include "Button.h"
#include "Control.h"
// --------------------------------------------------------------------------
/*! Вывод текущей информации на экран
 * - отображение состояния кабины лифта
 * - отображение состояния дверей
 * - отображение текущего положения кабины
 * - отображение этажей ждущих лифт
 * - вывод сообщений (в том числе от процесса управления)
 * - вывод "справки" о рабочих кнопках
 * - отображение текущих параметров "симуляции"
 *
 * Информация на экране обновляется каждые display_refresh_time
 * Все сообщения складываются в очередь, при отображении держаться на экране display_message_time, потом исчезают.
*/
class Display final:
	public so_5::agent_t
{
	public:
		Display( context_t ctx
				 , const std::string& name
				 , so_5::mbox_t control_mbox
				 , so_5::mbox_t status_mbox
				 , const std::vector<Button>& int_buttons // список кнопок внутри лифта
				 , const std::vector<Button>& ext_buttons // список кнопок на этажах
				 , const size_t nlevels // количество этажей
				 , const double height  // высота этажа
				 , const double speed   // скорость лифта
			   );

		virtual ~Display();

		/*! сообщение для отображения */
		struct msg_display_message:
			public so_5::message_t
		{
			std::string text;
			msg_display_message( const std::string& txt ):
				text(txt) {}
		};

		/*! обновление экрана */
		struct msg_refresh_t:
			public so_5::message_t {};

		/*! обновление сообщения */
		struct msg_update_text_t:
			public so_5::message_t {};

		/*! вычисление времени обновления информации на экране в зависимоти от скорости
		 * \param speed - скорость, м/c. Разрешённый диапазон [ 0.1 ... 20 ]
		 * \param height - высота этажа, м
		 * В случае ошибки генерирует исключение std::runtime_error
		 */
		static std::chrono::steady_clock::duration calc_refresh_time( const double& speed,
				const double& height );

	protected:
		virtual void so_define_agent() override;
		virtual void so_evt_start() override;

		void on_elevator_event( const Elevator::msg_state_t& m );
		void on_doors_event( const Doors::msg_state_t& m );
		void on_cpanel_event( const CPanel::msg_press_button_t& m );
		void on_control_event( const Control::msg_state_t& m );
		void on_display_message( const msg_display_message& m );
		void on_refresh( const msg_refresh_t& m );
		void on_update_text( const msg_update_text_t& m );

		void clear_screen();
		void display_status();
		void display_level();
		void display_buttons();
		void display_message();
		void display_hseparator();
		void display_parameters();

	private:
		const std::string myname;
		so_5::mbox_t control_mbox;
		so_5::mbox_t status_mbox;
		const std::vector<Button> int_buttons;
		const std::vector<Button> ext_buttons;

		// выбранные этажи (индесом является номер этажа! -1 делать не надо)
		std::vector<bool> selected_floors;
		std::vector<bool> selected_ext_buttons;
		std::vector<bool> selected_int_buttons;
		so_5::timer_id_t refresh_timer;
		std::queue<std::string> qmsg; /*!< очередь сообщений ожидающая показа */
		so_5::timer_id_t qmsg_timer;  /*!< таймер для отображения очередного сообщения */
		std::string text;  /*!< текущий отображаемый текст сообщения */

		// текущие состояния
		Elevator::msg_state_t elevator;
		Doors::msg_state_t doors;
		Control::msg_state_t control;

		const size_t nlevels;
		const double height;
		const double speed;

		/*! время обновления информации на экране
		 * Рассчитывается в зависимости от скорости лифта, с учётом того,
		 * что надо отображать прохождение каждого этажа (см. calc_refresh_time())
		 */
		const std::chrono::steady_clock::duration display_refresh_time;

		/*! время отображения очередного текстового сообщения */
		const std::chrono::steady_clock::duration display_message_time = { std::chrono::seconds(2) };
};
// --------------------------------------------------------------------------
#endif
// --------------------------------------------------------------------------
