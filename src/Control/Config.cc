#include <sstream>
#include "Config.h"
// --------------------------------------------------------------------------
using namespace std;
// --------------------------------------------------------------------------
void Config::check()
{
	if( levels < min_level || levels > max_level )
	{
		ostringstream err;
		err << "Bad number='" << levels << "' for levels."
			<< " Must be ["
			<< min_level << " .. " << max_level
			<< "]";
		throw std::runtime_error(err.str());
	}

	if( height < min_height || height > max_height )
	{
		ostringstream err;
		err << "Bad height='" << height << "' of the floor."
			<< " Must be ["
			<< min_height << " .. " << max_height
			<< "] meters" << endl;

		throw std::runtime_error(err.str());
	}

	if( speed < min_speed || speed > max_speed )
	{
		ostringstream err;
		err << "Bad elevator speed='" << speed << "'. "
			<< " Must be ["
			<< min_speed << " .. " << max_speed
			<< "] m/sec" << endl;

		throw std::runtime_error(err.str());
	}

	auto sec = chrono::duration_cast<chrono::seconds>(doors_time).count();

	if( sec < min_door_time || sec > max_door_time )
	{
		ostringstream err;
		err << "Bad value='" << sec << "' for close/open time for doors."
			<< " Must be ["
			<< min_door_time << " .. " << max_door_time
			<< "] sec" << endl;
		throw std::runtime_error(err.str());
	}

	sec = chrono::duration_cast<chrono::seconds>(waiting_time).count();

	if( sec < min_waiting_time || sec > max_waiting_time )
	{
		ostringstream err;
		err << "Bad value='" << sec << "' for loading/unloading time."
			<< " Must be ["
			<< min_waiting_time << " .. " << max_waiting_time
			<< "] sec" << endl;
		throw std::runtime_error(err.str());
	}

	if( threads < min_threads || threads > max_threads )
	{
		ostringstream err;
		err << "Bad value='" << threads << "' for threads."
			<< " Must be ["
			<< min_threads << " .. " << max_threads
			<< "]" << endl;
		throw std::runtime_error(err.str());
	}
}
