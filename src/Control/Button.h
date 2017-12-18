#ifndef Button_H_
#define Button_H_
// --------------------------------------------------------------------------
#include <limits>
#include <stddef.h>
// --------------------------------------------------------------------------
/*! Кнопка на этаже или внутри лифта */
struct Button
{
	enum ButtonType
	{
		btnUnknown,  /*!< неинициализированная кнопка */
		btnInternal, /*!< внутреняя кнопка в кабине */
		btnExternal  /*!< внешняя кнопка на этаже */
	};

	char symbol = { '\0' };
	size_t level = { 0 };
	ButtonType type = { btnUnknown };

	Button( char s, size_t l, ButtonType t )
		: symbol(s)
		, level(l)
		, type(t)
	{}

	// проверка есть ли очередная команда (кнопка)
	operator bool() const
	{
		return ( type == btnUnknown );
	}
};
// --------------------------------------------------------------------------
#endif // end of Button_H_
// --------------------------------------------------------------------------
