#include <iostream>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

#include "Voodoo.h"
#include "VoodooTest.h"


class IClock : public Voodoo::InterfaceClient
{
public:
	class Time
	{
	private:
		unsigned int hours;
		unsigned int minutes;
		unsigned int seconds;

	public:
		Time(sf::Int64 _seconds)
		{
			minutes = (unsigned int)(_seconds / 60);

			seconds = _seconds % 60;

			hours = minutes / 60;

			minutes %= 60;
		}

		unsigned int GetHours() const { return hours; }
		unsigned int GetMinutes() const { return minutes; }
		unsigned int GetSeconds() const { return seconds; }
	};

public:
	typedef enum {
		GET_TIME = RELEASE + 1,
		SET_TIME
	} Method;

public:
	IClock(Voodoo::Client& client, Voodoo::ID method_id)
		:
		InterfaceClient(client, method_id)
	{
	}

	Time GetTime()
	{
		auto result = client.Call(method_id, (int)GET_TIME);

		return Time(std::any_cast<sf::Int64>(result[0]));
	}

	void SetTime( const Time &time )
	{
		client.Call(method_id, (int)SET_TIME, time.GetHours(), time.GetMinutes(), time.GetSeconds());
	}
};


class IClock_Server
{
private:
	Voodoo::Server& server;
	Voodoo::ID method_id;
	sf::Int64 time_offset;

public:
	IClock_Server(Voodoo::Server& server)
		:
		server(server),
		time_offset(0)
	{
		method_id = server.Register([&server, this](std::vector<std::any> args)
			{
				std::any ret = 0;

				int method = std::any_cast<int>(args[0]);

				sf::Int64 current = get_current_time();

				switch (method) {
				case IClock::RELEASE:
					delete this;
					break;
				case IClock::GET_TIME:
					ret = current + time_offset;
					break;
				case IClock::SET_TIME:
					time_offset =
						std::any_cast<unsigned int>(args[1]) * 60 * 60 +
						std::any_cast<unsigned int>(args[2]) * 60 +
						std::any_cast<unsigned int>(args[3]);
					time_offset -= current;
					break;
				}

				return ret;
			});
	}

	~IClock_Server()
	{
		server.Unregister(method_id);
	}

	Voodoo::ID GetMethodID() const
	{
		return method_id;
	}

private:
	sf::Int64 get_current_time()
	{
#ifdef _WIN32
		SYSTEMTIME local;

		GetLocalTime(&local);

		//std::cout << "wHour " << local.wHour << std::endl;
		//std::cout << "wMinute " << local.wMinute << std::endl;
		//std::cout << "wSecond " << local.wSecond << std::endl;

		return local.wHour * 60 * 60 + local.wMinute * 60 + local.wSecond;
#else
		struct timeval tv;

		gettimeofday(&tv, NULL);

		return tv.tv_sec % (24 * 60 * 60);
#endif
	}
};



int main()
{
	Voodoo::Server server;
	Voodoo::Client client;

	VoodooTest::Setup setup(server, client);


	Voodoo::ID clock_id = 1;	// In this case we know the ID that is used onb the server to register

	std::unique_ptr<std::thread> server_loop;

	if (setup.test_server) {
		clock_id = server.Register([&server](std::vector<std::any> args)
			{
				auto clock = new IClock_Server(server);

				return clock->GetMethodID();
			});

		server_loop = std::make_unique<std::thread>([&server]() {
				server.Run();
			});
	}


	if (setup.test_client) {
		auto result = client.Call(clock_id);

		auto clock = new IClock(client, std::any_cast<Voodoo::ID>(result[0]));

		IClock::Time time = clock->GetTime();

		std::cout << "Time: " << time.GetHours() << " hours, " << time.GetMinutes() << " minutes, " << time.GetSeconds() << " seconds" << std::endl;


		clock->SetTime(IClock::Time(7 * 60 * 60));

		time = clock->GetTime();

		std::cout << "Time: " << time.GetHours() << " hours, " << time.GetMinutes() << " minutes, " << time.GetSeconds() << " seconds" << std::endl;


		delete clock;
	}
	else
		setup.wait_server();


	if (setup.test_server)
		server.Stop();

	if (server_loop)
		server_loop->join();
	
	return 0;
}
