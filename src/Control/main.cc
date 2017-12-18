// ----------------------------------------------------------------------------
#include <iostream>
#include <sstream>
#include <so_5/all.hpp>
#include <iomanip>
#include "clipp.h"
#include "Display.h"
#include "CPanel.h"
#include "Elevator.h"
#include "Doors.h"
#include "Control.h"
#include "Config.h"
// ----------------------------------------------------------------------------
using namespace std;
using namespace clipp;
// ----------------------------------------------------------------------------
static std::string help_text( const std::string& txt
							  , const double& defval
							  , const double& min
							  , const double& max )
{
	ostringstream s;
	s << txt << "[" << min << " .. " << max << "]. Default: " << defval;
	return s.str();
}
// ----------------------------------------------------------------------------
static std::string help_text( const std::string& txt
							  , const size_t& defval
							  , const size_t& min
							  , const size_t& max )
{
	ostringstream s;
	s << txt << "[" << min << " .. " << max << "]. Default: " << defval;
	return s.str();
}
// ----------------------------------------------------------------------------
int main( int argc, char* argv[] )
{
	Config cfg;
	bool help = false;
	size_t door_sec = 3;
	size_t waiting_sec = 10;

	// кажется clipp не умеет отображать значение по умолчанию и диапазон
	// поэтому используем свою функцию форматирования help_text()

	auto cli = (
				   (option("-l", "--levels") & value("num").set(cfg.levels))    % help_text("Number of levels", cfg.levels, cfg.min_level, cfg.max_level),
				   (option("-h", "--height") & value("meters").set(cfg.height)) % help_text("Height of the floor", cfg.height, cfg.min_height, cfg.max_height),
				   (option("-s", "--speed") & value("m/sec").set(cfg.speed))    % help_text("Elevator speed", cfg.speed, cfg.min_speed, cfg.max_speed),
				   (option("-d", "--doors-time") & value("sec").set(door_sec))  % help_text("Time for close/open the doors", door_sec, cfg.min_door_time, cfg.max_door_time),
				   (option("-w", "--waiting-time") & value("sec").set(waiting_sec)) % help_text("Time for loading/unloading", waiting_sec, cfg.min_waiting_time, cfg.max_waiting_time),
				   (option("-t", "--threads") & value("num").set(cfg.threads)) % help_text("Number of threads", cfg.threads, cfg.min_threads, cfg.max_threads),
				   (option("--help").set(help)) % "Help message"
			   );

	try
	{

		if( !parse(argc, argv, cli) )
		{
			cout << make_man_page(cli, argv[0]);
			return 1;
		}

		if( help )
		{
			cout << make_man_page(cli, argv[0]);
			return 0;
		}

		cfg.doors_time = std::chrono::seconds{door_sec};
		cfg.waiting_time = std::chrono::seconds{waiting_sec};

		cfg.check();

		// создаём кнопки с учётом конфигурации
		int ascii_a = (int)'a';

		cfg.int_buttons.reserve(cfg.levels);

		for( size_t i = 1; i <= cfg.levels; i++ )
			cfg.int_buttons.push_back( Button(ascii_a + i - 1, i, Button::btnInternal) );

		cfg.ext_buttons.reserve(cfg.levels);

		for( size_t i = 1; i <= cfg.levels; i++ )
			cfg.ext_buttons.push_back( Button(std::toupper(ascii_a + i - 1), i, Button::btnExternal) );

		so_5::launch( [&cfg]( so_5::environment_t& env )
		{
			// используем пул потоков (и у каждого агента своя очередь)
			// см. https://sourceforge.net/p/sobjectizer/wiki/so-5.5%20In-depth%20-%20Dispatchers/
			env.introduce_coop( "control_coop",
								so_5::disp::thread_pool::create_private_disp( env, cfg.threads )->binder(
									so_5::disp::thread_pool::bind_params_t{} .fifo( so_5::disp::thread_pool::fifo_t::individual ) ),
								[&cfg, &env]( so_5::coop_t& coop )
			{
				// создаём публичные ящики
				auto pub_status_mbox = env.create_mbox("pub_status_mbox");
				auto pub_command_mbox = env.create_mbox("pub_command_mbox");

				// можно было бы передавать в конструкторе Config
				// но пусть зависимости будут видны явно
				coop.make_agent< Display >("display", pub_command_mbox, pub_status_mbox
										   , cfg.int_buttons
										   , cfg.ext_buttons
										   , cfg.levels
										   , cfg.height
										   , cfg.speed );

				coop.make_agent< Control >("control", pub_command_mbox, pub_status_mbox
										   , cfg.doors_time
										   , cfg.waiting_time );

				coop.make_agent< CPanel >("cpanel", pub_command_mbox, pub_status_mbox
										  , cfg.int_buttons
										  , cfg.ext_buttons );

				coop.make_agent< Elevator >("elevator", pub_command_mbox, pub_status_mbox
											, cfg.levels
											, cfg.height
											, cfg.speed );

				coop.make_agent< Doors >("doors", pub_command_mbox, pub_status_mbox
										 , cfg.doors_time );
			});
			//		},
			//		[]( so_5::environment_params_t & params ) {
			//			params.message_delivery_tracer( so_5::msg_tracing::std_cout_tracer() );
		});
	}
	catch( const std::exception& ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
