#ifndef CPanel_H_
#define CPanel_H_
// --------------------------------------------------------------------------
#include <string>
#include <sstream>
#include <so_5/all.hpp>
#include "Button.h"
// --------------------------------------------------------------------------
/*! Взаимодествие с кнопками управления (ввод пользователя)
 * Считаем: "одна клавиша"(один символ) - "одна кнопка".
 * После нажатия 'enter' проверяет входит ли последний введённый символ
 * в диапазон управляющих и отправляет команду в Control для обработки.
 * Ожидание ввода - не блокирующее.
*/
class CPanel final:
	public so_5::agent_t
{
	public:
		CPanel( context_t ctx
				, const std::string& name
				, so_5::mbox_t control_mbox
				, so_5::mbox_t status_mbox
				, const std::vector<Button>& in_buttons  // список кнопок внутри лифта
				, const std::vector<Button>& ext_buttons // список кнопок на этажах
			  );

		virtual ~CPanel();

		struct msg_press_button_t:
			public so_5::message_t
		{
			Button btn;
			msg_press_button_t( const Button& b )
				: btn(b)
			{}
		};

		/*! проверка текущей позиции */
		struct msg_read_input_t:
			public so_5::message_t {};

	protected:
		virtual void so_define_agent() override;
		virtual void so_evt_start() override;
		virtual void so_evt_finish() override;

		void on_read_input( const msg_read_input_t& m );

		void do_send_message( const std::string& txt );
		void do_send_command( const Button& btn );

		Button get_button_by_code( const char btn ) const;

	private:
		const std::string myname;
		so_5::mbox_t control_mbox;
		so_5::mbox_t status_mbox;
		int oldflags; // flags for input
		so_5::timer_id_t readinput_timer;
		const std::vector<Button> in_buttons;
		const std::vector<Button> ext_buttons;
		// время ожидания события ввода
		// (не должно быть очень большим)
		// для простоты делаем не настраиваемым
		const time_t select_timeout_sec = { 5 };
};
// --------------------------------------------------------------------------
#endif // end of CPanel
// --------------------------------------------------------------------------
