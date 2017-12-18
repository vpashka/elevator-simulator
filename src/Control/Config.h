#ifndef Config_H_
#define Config_H_
// --------------------------------------------------------------------------
#include <vector>
#include <chrono>
#include "Button.h"
// --------------------------------------------------------------------------
struct Config
{
	// задаваемые параметры
	size_t levels = { 20 };  /*!< количество этажей */
	double height = { 2.0 }; /*!< высота этажа, м */
	double speed = { 1.0 };  /*!< скорость, м/с */

	/*! время между открытием и закрытием дверей (время ожидания погрузки и выгрузки) */
	std::chrono::steady_clock::duration waiting_time = { std::chrono::seconds(10) };

	// дополнительные параметры
	// -----------------------------
	/*! время открытия/закрытия дверей, сек */
	std::chrono::steady_clock::duration doors_time = { std::chrono::seconds(3) };

	// список кнопок
	std::vector<Button> int_buttons; /*!< кнопки внутри лифта */
	std::vector<Button> ext_buttons; /*!< кнопки снаружи лифта (на этажах) */

	/*! количество потоков для работы
	 *  \note фактическое количество потоков будет +3, необходимые для работы sobjectizer
	 */
	size_t threads = { 3 };

	// Ограничительные константы
	const size_t max_level = { 20 };
	const size_t min_level = { 5 };
	const double max_height = { 10.0 };
	const double min_height = { 2.0 };
	const size_t max_door_time = { 15 };
	const size_t min_door_time = { 1 };
	const double max_speed = { 10 };
	const double min_speed = { 0.1 };
	const size_t max_waiting_time = { 60 };
	const size_t min_waiting_time = { 2 };
	const size_t max_threads = { 5 };
	const size_t min_threads = { 2 }; // минимум два, чтобы всегда был отдельный поток для пользовательского ввода

	void check(); // throw runtime_error()
};
// --------------------------------------------------------------------------
#endif // end of Config
// --------------------------------------------------------------------------
