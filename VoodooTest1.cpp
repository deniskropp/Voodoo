#include <iostream>

#include <windows.h>

#include "Voodoo.h"


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
		std::any result = client.Call(method_id, (int)GET_TIME);

		return Time(std::any_cast<sf::Int64>(result));
	}

	void SetTime( const Time &time )
	{
//		client.Call(method_id, (int)SET_TIME, time.GetHours(), time.GetMinutes(), time.GetSeconds());
		client.Call(method_id, (int)SET_TIME, time);
	}
};

template <>
void Voodoo::Host::put_arg(sf::Packet& packet, IClock::Time arg)
{
	packet << TimeType::number;
	packet << arg.GetHours();
	packet << arg.GetMinutes();
	packet << arg.GetSeconds();
}


class IClock_Server
{
private:
	Voodoo::ID method_id;
	sf::Int64 time_offset;

public:
	IClock_Server(Voodoo::Server& server)
		:
		time_offset(0)
	{
		method_id = server.Register([&server, this](std::vector<std::any> args)
			{
				std::any ret = 0;

				int method = std::any_cast<int>(args[0]);

				sf::Int64 current = get_current_time();

				switch (method) {
				case IClock::RELEASE:
					server.Unregister(method_id);
					delete this;
					break;
				case IClock::GET_TIME:
					//ret = current + time_offset;
					ret = IClock::Time(current + time_offset);
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

	Voodoo::ID GetMethodID() const
	{
		return method_id;
	}

private:
	sf::Int64 get_current_time()
	{
		SYSTEMTIME local;

		GetLocalTime(&local);

		//std::cout << "wHour " << local.wHour << std::endl;
		//std::cout << "wMinute " << local.wMinute << std::endl;
		//std::cout << "wSecond " << local.wSecond << std::endl;

		return local.wHour * 60 * 60 + local.wMinute * 60 + local.wSecond;
	}
};



static void
print_any(std::any value, std::string prefix = "")
{
	if (value.type() == typeid(Voodoo::ID))
		std::cout << prefix << "    :" << std::any_cast<Voodoo::ID>(value).value << std::endl;
	else if (value.type() == typeid(int))
		std::cout << prefix << "    " << std::any_cast<int>(value) << std::endl;
	else if (value.type() == typeid(std::string))
		std::cout << prefix << "    \"" << std::any_cast<std::string>(value) << "\"" << std::endl;
}


int main()
{
	Voodoo::Server server;
	Voodoo::Client client;

	Voodoo::ID clock_id = server.Register([&server](std::vector<std::any> args)
		{
			for (auto arg : args)
				print_any(arg);

			auto clock = new IClock_Server(server);

			return clock->GetMethodID();
		});

	std::thread server_loop([&server]() { server.Run(); });


	std::any result;
	
	result = client.Call(clock_id);


	auto clock = new IClock(client, std::any_cast<Voodoo::ID>(result));

	IClock::Time time = clock->GetTime();

	std::cout << "Time: " << time.GetHours() << " hours, " << time.GetMinutes() << " minutes, " << time.GetSeconds() << " seconds" << std::endl;


	clock->SetTime(IClock::Time(7 * 60 * 60));

	time = clock->GetTime();

	std::cout << "Time: " << time.GetHours() << " hours, " << time.GetMinutes() << " minutes, " << time.GetSeconds() << " seconds" << std::endl;


	delete clock;


	server.Stop();

	server_loop.join();
	
	return 0;
}