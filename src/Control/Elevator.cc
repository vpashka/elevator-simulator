#include <cmath>
#include "Elevator.h"
// --------------------------------------------------------------------------
using namespace std;
// --------------------------------------------------------------------------
Elevator::Elevator(context_t _ctx
				   , const std::string& _name
				   , so_5::mbox_t _control_mbox
				   , so_5::mbox_t _status_mbox
				   , size_t _nlevels
				   , double _height
				   , double _speed
				  )
	: so_5::agent_t{ _ctx }
	, myname(_name)
	, control_mbox(std::move(_control_mbox))
	, status_mbox(std::move(_status_mbox))
	, nlevels(_nlevels)
	, height(_height)
	, maxposition( (_nlevels - 1) * _height)
	, speed(_speed)
	, step_time(Elevator::calc_step_time(_speed))
{
	level = calc_level_number(position, height);
}
// --------------------------------------------------------------------------
Elevator::~Elevator()
{
}
// --------------------------------------------------------------------------
double Elevator::calc_position_step( const double& _speed,
									 const std::chrono::steady_clock::duration& _step_time )
{
	// вычисление сколько проезжаем за _step_time
	// со скоростью _speed
	// в формате double
	// по условиям нам должно хватать точности microseconds (см. calc_step_time)
	using namespace std::chrono;

	auto microsec = duration_cast<microseconds>(_step_time).count();
	return ( _speed * (double)microsec ) / 1000000.;
}
// --------------------------------------------------------------------------
size_t Elevator::calc_level_number( const double& _position, const double& _height )
{
	// этажи считаем с первого, а не с нулевого
	return lroundf(_position / _height) + 1;
}
// --------------------------------------------------------------------------
std::chrono::steady_clock::duration Elevator::calc_step_time( const double& speed )
{
	if( speed > 20.0 )
	{
		ostringstream err;
		err << "(Elevator::calc_step_time): BAD speed= " << speed << " Must be < 20.0";
		throw std::runtime_error(err.str());
	}

	if( speed < 0.1 )
	{
		ostringstream err;
		err << "(Elevator::calc_step_time): BAD speed= " << speed << " Must be > 0.1";
		throw std::runtime_error(err.str());
	}

	// Считаем, что исполнение команд мгновенно!
	// (в реальности мы бы начинали снижение скорости зараннее)
	// Поэтому, для соблюдения точности позиционирования до 1 см достаточно 10 милисек
	// 1 м/с --> 1 см. в 10 миллисек
	// (в реальности, нужно было бы увеличить точность на порядок, чтобы снимать несколько точек
	// во время движения)

	// переводим скорость м/с --> см/микросек
	return chrono::microseconds( lroundf(10000. / speed) );
}
// --------------------------------------------------------------------------
void Elevator::so_define_agent()
{
	so_subscribe(control_mbox).event( &Elevator::cmd_move_to );
	so_subscribe_self()
	.event( &Elevator::get_state )
	.event( &Elevator::step );
}
// --------------------------------------------------------------------------
void Elevator::so_evt_start()
{
	// начальная инициализация, рассылаем всем своё состояние
	so_5::send< msg_state_t >(status_mbox, level, position, state);
}
// --------------------------------------------------------------------------
Elevator::msg_state_t Elevator::get_state( const Elevator::msg_get_state_t& m )
{
	return msg_state_t(level, position, state);
}
// --------------------------------------------------------------------------
void Elevator::cmd_move_to( const Elevator::msg_move_to_level_t& m )
{
	double new_target = ((double)(m.level - 1) * height);

	// игнорируем некорректное задание
	// тут надо писать в лог ошибку
	if( new_target < 0.0f || new_target > maxposition )
		return;

	double prev_target = target;
	target = new_target;

	// мы уже на месте
	if( fabs(target - prev_target) < precision )
		return;

	if( target > position )
		go_up();
	else
		go_down();
}
// --------------------------------------------------------------------------
void Elevator::go_up()
{
	if( level >= nlevels )
		return;

	state = State::elMoveUp;

	if( !step_timer.is_active() )
	{
		step_timer = so_5::send_periodic< msg_check_position_t >(
						 *this,
						 std::chrono::seconds::zero(),
						 // повтор сообщения каждые step_time
						 step_time );
	}

	so_5::send< msg_state_t >(status_mbox, level, position, state);
}
// --------------------------------------------------------------------------
void Elevator::go_down()
{
	if( position <= precision )
		return;

	state = State::elMoveDown;

	if( !step_timer.is_active() )
	{
		step_timer = so_5::send_periodic< msg_check_position_t >(
						 *this,
						 std::chrono::seconds::zero(),
						 step_time );
	}

	so_5::send< msg_state_t >(status_mbox, level, position, state);
}
// --------------------------------------------------------------------------
void Elevator::step( const Elevator::msg_check_position_t& m )
{
	double position_step_sm = calc_position_step(speed, step_time);

	if( state == State::elMoveDown )
	{
		position -= position_step_sm;

		if( position <= precision )
		{
			position = 0;
			state = State::elStopped;
			step_timer.release();
		}
	}
	else if( state == State::elMoveUp )
	{
		position += position_step_sm;

		if( fabs(maxposition - position) <= precision )
		{
			level = maxposition;
			state = State::elStopped;
			step_timer.release();
		}
	}

	if( fabs(target - position) <= precision )
	{
		// обновляем цель, чтобы потом задание не скакало
		target = position;
		state = State::elStopped;
		step_timer.release();
	}

	level = calc_level_number(position, height);

	// публикуем состояние
	so_5::send< msg_state_t >(status_mbox, level, position, state);
}
// --------------------------------------------------------------------------
namespace std
{
	std::string to_string( const Elevator::State& s )
	{
		if( s == Elevator::State::elStopped )
			return "Stopped";

		if( s == Elevator::State::elMoveDown )
			return "MoveDown";

		if( s == Elevator::State::elMoveUp )
			return "MoveUp";

		return "";
	}
}
// --------------------------------------------------------------------------
