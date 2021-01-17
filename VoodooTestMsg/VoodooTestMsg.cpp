#include <array>
#include <iostream>
#include <queue>
#include <set>

#include "Voodoo.h"
#include "VoodooTest.h"


class IMsg : public Voodoo::InterfaceClient
{
public:
	using Method = enum {
		RELEASE,

		RECV_MSG,
		SEND_MSG,

		_NUM_METHODS
	};

public:
	IMsg(Voodoo::Client& client, Voodoo::ID method_id)
		:
		InterfaceClient(client, method_id)
	{
	}

	std::string RecvMsg()
	{
		auto result = client.Call(method_id, (int)RECV_MSG);

		return std::any_cast<std::string>(result[0]);
	}

	void SendMsg(std::string msg)
	{
		client.Call(method_id, (int)SEND_MSG, msg);
	}
};


class Member
{
public:
	virtual void PutLine(std::string text) = 0;
};

class Room
{
private:
	std::set<Member*> members;

public:
	void Enter(Member* member)
	{
		members.insert(member);
	}

	void Write(const std::string& text)
	{
		for (auto m : members)
			m->PutLine(text);
	}
};


class IMsg_Server : public Voodoo::InterfaceServer<IMsg>, public Member
{
private:
	Room& room;
	std::array<std::function<std::any(std::vector<std::any>)>, IMsg::_NUM_METHODS> dispatch;
	std::queue<std::string> messages;

public:
	virtual void PutLine(std::string text)
	{
		messages.push(text);
	}

public:
	IMsg_Server(Voodoo::Server& server, Room &room)
		:
		InterfaceServer(server),
		room(room)
	{
		dispatch[IMsg::RECV_MSG] = [this](std::vector<std::any> args) -> std::any
		{
			if (messages.empty())
				return std::string();

			std::string msg = messages.front();

			messages.pop();

			return msg;
		};

		dispatch[IMsg::SEND_MSG] = [&room](std::vector<std::any> args) -> std::any
		{
			room.Write(std::any_cast<std::string>(args[1]));

			return 0;
		};
	}

	virtual std::function<std::any(std::vector<std::any>)> Lookup(IMsg::Method method) const
	{
		return dispatch[method];
	}
};



int main()
{
	Room room;

	Voodoo::Server server;
	Voodoo::Client client;

	VoodooTest::Setup setup(server, client);


	Voodoo::ID msg_id = 1;	// In this case we know the ID that is used onb the server to register

	std::unique_ptr<std::thread> server_loop;

	if (setup.test_server) {
		msg_id = server.Register([&server,&room](std::vector<std::any> args)
			{
				auto msg = new IMsg_Server(server, room);

				room.Enter(msg);

				return msg->GetMethodID();
			});

		server_loop = std::make_unique<std::thread>([&server]()
			{
				server.Run();
			});
	}


	if (setup.test_client) {
		auto result = client.Call(msg_id);

		auto msg = new IMsg(client, std::any_cast<Voodoo::ID>(result[0]));

		while (true) {
			while (true) {
				std::string message = msg->RecvMsg();

				if (message.empty())
					break;

				std::cout << "> \"" << message << "\"" << std::endl;
			}

			char text[100];

			std::cout << "> ";
			std::cin.getline(text, 100);

			msg->SendMsg(text);
		}


		delete msg;
	}
	else
		setup.wait_server();


	if (setup.test_server)
		server.Stop();

	if (server_loop)
		server_loop->join();

	return 0;
}
