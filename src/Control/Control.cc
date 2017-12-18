#include "Elevator.h"
#include "Control.h"
// --------------------------------------------------------------------------
using namespace std;
// --------------------------------------------------------------------------
Control::Control(context_t ctx
				 , const std::string& name
				 , so_5::mbox_t _control_mbox
				 , so_5::mbox_t _status_mbox
				 , std::chrono::steady_clock::duration _doors_time
				 , std::chrono::steady_clock::duration _waiting_time )
	: so_5::agent_t{ ctx }
	, myname(name)
	, control_mbox(std::move(_control_mbox))
	, status_mbox(std::move(_status_mbox))
	, elevator(0, 0, Elevator::State::elStopped)
	, doors_time(_doors_time)
	, waiting_time(_waiting_time)
{

}
// --------------------------------------------------------------------------
Control::~Control()
{
}
// --------------------------------------------------------------------------
void Control::so_define_agent()
{
	// ----------------------------------------------
	// общие обработчики
	// ----------------------------------------------
	st_processing.on_enter([this] {so_5::send< msg_state_t >(status_mbox, State::stProcessing); });
	st_standby.on_enter([this] {so_5::send< msg_state_t >(status_mbox, State::stStandBy); });


	// если мы в процессе погрузки/выгрузки
	// то команды просто складываем в очередь
	so_subscribe(control_mbox)
	.in(st_processing)
	.event([this](const CPanel::msg_press_button_t& m)
	{
		save_command(m);
	});

	// если уже едем, то сразу исполняем (чтобы проверить не появилась ли новая цель "по пути")
	so_subscribe(control_mbox)
	.in(st_moving)
	.event([this](const CPanel::msg_press_button_t& m)
	{

		save_command(m);
		do_command();
	});

	// если в режиме standby то сразу переходим к движению на этаж который вызвал лифт
	// (либо если мы уже на нём, то открываем двери. см. do_command() )
	so_subscribe(control_mbox)
	.in(st_standby)
	.event([this](const CPanel::msg_press_button_t& m)
	{

		save_command(m);
		st_moving.activate();
	});

	st_moving.on_enter([this]
	{
		so_5::send< msg_state_t >(status_mbox, State::stMoving);
		// т.к. в обработчике on_enter считается что мы ещё не находимся в st_moving
		// посылаем себе отложенное сообщение (т.к. в do_command() есть проверка на st_moving)
		so_5::send_delayed<msg_delayed_action_t>(*this, chrono::seconds::zero());
	}).event([this]( const msg_delayed_action_t& m )
	{
		do_command();
	});

	// -------------------------------------------------------------------
	// расписываем состояния и переходы между ними
	// -------------------------------------------------------------------
	so_subscribe(status_mbox)
	.in(st_moving)
	.event([this](const Elevator::msg_state_t& m )
	{
		elevator = m;

		// если лифт остановился, считаем что приехали
		// начинаем "погрузку/выгрузку"
		// аварийные остановки не рассматриваем в этом проекте
		if( m.state == Elevator::State::elStopped )
		{
			// удаляем из очереди все команды требующие текущий этаж
			remove_targets( elevator.level );
			// начинаем процесс погрузки/выгрузки
			st_processing.activate();
		}
	});

	// в режимах stanby и processing в событиях от Elevator, просто обновляем состояние
	so_subscribe(status_mbox)
	.in(st_standby)
	.in(st_processing)
	.event([this]( const Elevator::msg_state_t& m )
	{
		elevator = m;
	});

	st_opening.on_enter([this]
	{
		// подаём команду на открытие дверей
		so_5::send<Doors::msg_open_t>(control_mbox);
	});

	so_subscribe(status_mbox)
	.in(st_opening)
	.event([this](const Doors::msg_state_t& m )
	{

		// если двери открылись, переходим в состояние ожидания
		if( m.state == Doors::State::dOpened )
			st_waiting.activate();
	});
	// здесь ещё можно отработать timeout если двери таки и не открылись
	// но это уже "нештатная ситуация" которые мы не рассматриваем
	// но выглядело бы это примерно так:
	// .time_limit(doors_processing_time,st_failed);


	// Для простоты демо-проекта мы просто ждём wait_processing_time
	// погрузку/выгрузку и закрываем двери
	// в реальности надо отрабатывать датчики движения
	st_waiting.time_limit(waiting_time, st_closing);

	st_closing.on_enter([this]
	{
		// подаём команду на закрытие дверей
		so_5::send<Doors::msg_close_t>(control_mbox);
	});

	so_subscribe(status_mbox)
	.in(st_closing)
	.event([this](const Doors::msg_state_t& m )
	{

		if( m.state == Doors::State::dClosed )
		{
			if( targets.empty() )
				st_standby.activate();
			else
			{
				if( st_moving.is_active() )
					do_command();
				else
					st_moving.activate();
			}
		}
	});
}
// --------------------------------------------------------------------------
void Control::so_evt_start()
{
	st_standby.activate();
}
// --------------------------------------------------------------------------
void Control::save_command( const CPanel::msg_press_button_t& m )
{
	// если мы "грузим/выгружаем" на текущем этаже, то добавлять в список не надо
	// а сразу открываем двери, если уже ждём закрытия
	// а если двери и так открыты.. то ничего не делаем
	if( st_processing.is_active() && m.btn.level == elevator.level )
	{
		if( st_closing.is_active() )
			st_opening.activate();

		return;
	}

	// добавляем очередное задание и пересортировываем список в зависимости от движения
	targets.push_back(m.btn);
	sort_targets(targets, elevator.state, elevator.level);
}
// --------------------------------------------------------------------------
void Control::sort_targets( std::list<Button>& tlist, Elevator::State estate, size_t current_level )
{
	if( tlist.empty() )
		return;

	// базовые значения относительно которых сортируется список
	Elevator::State base_state = estate;

	// если лифт стоит, то команды отрабатываются в порядке поступления
	// поэтому смотрим первую команду, понимаем по ней куда собираемся ехать
	// и под эту цель, перестраиваем список
	// переопределяя base_state
	if( estate == Elevator::State::elStopped )
	{
		auto cmd = tlist.begin();

		if( cmd->level > current_level )
			base_state = Elevator::State::elMoveUp;
		else // if( cmd.level <= current_level )
			base_state = Elevator::State::elMoveDown;
	}

	// При движении наверх, все задания выше или равные текущему этажу
	// должны быть вначале (остальные потом)
	// Если мы едем вниз, то все задания меньше или равные нашему этажу
	// должны быть в начале
	// список заданий при этом сортируется в порядке возрастания или убывания
	// в зависимости направления движения

	// Делаем в два прохода:
	// --------------------------------------
	// 1. Сортируем просто по возрастанию или убыванию (в зависимости от движения)
	tlist.sort([&base_state]( const Button & b1, const Button & b2 )
	{

		if( base_state == Elevator::State::elMoveUp )
			return b1.level < b2.level;

		// если движемся вниз, сортируем по убыванию
		return b2.level < b1.level;
	});

	// 2. делаем stable_partition, чтобы пересортировать относительно текущего этажа
	std::stable_partition(tlist.begin(), tlist.end(), [&base_state, &current_level]( const Button & btn )
	{

		if( base_state == Elevator::State::elMoveDown )
			return btn.level <= current_level;

		// здесь if уже не нужен, т.к. нет других вариантов
		// if( elevator.state == Elevator::State::elMoveUp )
		return btn.level >= current_level;
	});

	// порядок в оставшейся части нас "пока" не интересует, т.к. после каждой остановки
	// мы пересортировываем заново
	// хотя можно было бы и эту часть отсортировать "в обратном движению порядке"
}
// --------------------------------------------------------------------------
void Control::do_command()
{
	if( targets.empty() )
		return;

	// команда на движение должна вызываться
	// только в режиме 'movig'
	if( !st_moving.is_active() )
		return;

	sort_targets(targets, elevator.state, elevator.level);

	// каждый раз смотрим первый элемент
	// но удаляем исполненнyю команду только когда доехали до места
	// см. обработку Elevator::msg_state_t в режиме st_moving
	auto cmd = targets.begin();

	// если мы уже на заданном этаже, переходим к погрузке/выгрузке
	if( elevator.level == cmd->level )
	{
		remove_targets( elevator.level );
		st_processing.activate();
	}
	else // иначе едем к нужному
		so_5::send<Elevator::msg_move_to_level_t>(control_mbox, cmd->level);
}
// --------------------------------------------------------------------------
void Control::remove_targets( size_t level )
{
	targets.remove_if([&level](const Button & b)
	{
		return ( b.level == level );
	});
}
// --------------------------------------------------------------------------
namespace std
{
	std::string to_string( const Control::State& s )
	{
		if( s == Control::State::stStandBy )
			return "StandBy";

		if( s == Control::State::stProcessing )
			return "Processing";

		if( s == Control::State::stMoving )
			return "Moving";

		return "?Unknown?";
	}
}
// --------------------------------------------------------------------------
