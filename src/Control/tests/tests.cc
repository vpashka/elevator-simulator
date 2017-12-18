#define CATCH_CONFIG_RUNNER
#include <catch.hpp>
#include <so_5/all.hpp>
// --------------------------------------------------------------------------
using namespace std;
// --------------------------------------------------------------------------
so_5::wrapped_env_t global_env; // делаем глобальным для всех тестов
// --------------------------------------------------------------------------
int main( int argc, char* argv[] )
{
	try
	{
		Catch::Session session;

        int returnCode = session.applyCommandLine( argc, argv );

        if( returnCode != 0 )
			return returnCode;

        returnCode = session.run();
        global_env.stop_then_join();
        return returnCode;
	}
	catch( const std::exception& e )
	{
		cerr << "(tests): " << e.what() << endl;
	}
	catch(...)
	{
		cerr << "(tests): catch(...)" << endl;
	}

    global_env.stop_then_join();
	return 1;
}
