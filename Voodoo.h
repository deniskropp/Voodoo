#include <assert.h>

#include <any>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <thread>

#include <SFML/Network.hpp>


namespace Voodoo {


class ID
{
public:
	sf::Uint64 value;

	ID();
	ID(sf::Uint64 value);

	sf::Uint64 operator *() const { return value; }

	bool operator < (const ID& other) const;
};


sf::Packet& operator <<(sf::Packet& packet, const ID& id);
sf::Packet& operator >>(sf::Packet& packet, ID& id);


class Packet
{
public:
	typedef enum {
		ID,
		INT8,
		UINT8,
		INT16,
		UINT16,
		INT32,
		UINT32,
		INT64,
		UINT64,
		FLOAT32,
		FLOAT64,
		STRING,
		DATA
	} ValueType;
};


namespace {
	template <typename T>
	static void put_arg(sf::Packet& packet, T arg);

	template <>
	void put_arg(sf::Packet& packet, ID arg)
	{
		packet << Packet::ID;
		packet << arg;
	}

	template <>
	void put_arg(sf::Packet& packet, sf::Int8 arg)
	{
		packet << Packet::INT8;
		packet << arg;
	}

	template <>
	void put_arg(sf::Packet& packet, sf::Uint8 arg)
	{
		packet << Packet::UINT8;
		packet << arg;
	}

	template <>
	void put_arg(sf::Packet& packet, sf::Int16 arg)
	{
		packet << Packet::INT16;
		packet << arg;
	}

	template <>
	void put_arg(sf::Packet& packet, sf::Uint16 arg)
	{
		packet << Packet::UINT16;
		packet << arg;
	}

	template <>
	void put_arg(sf::Packet& packet, sf::Int32 arg)
	{
		packet << Packet::INT32;
		packet << arg;
	}

	template <>
	void put_arg(sf::Packet& packet, sf::Uint32 arg)
	{
		packet << Packet::UINT32;
		packet << arg;
	}

	template <>
	void put_arg(sf::Packet& packet, sf::Int64 arg)
	{
		packet << Packet::INT64;
		packet << arg;
	}

	template <>
	void put_arg(sf::Packet& packet, sf::Uint64 arg)
	{
		packet << Packet::UINT64;
		packet << arg;
	}

	template <>
	void put_arg(sf::Packet& packet, float arg)
	{
		packet << Packet::FLOAT32;
		packet << arg;
	}

	template <>
	void put_arg(sf::Packet& packet, double arg)
	{
		packet << Packet::FLOAT64;
		packet << arg;
	}

	template <>
	void put_arg(sf::Packet& packet, std::string arg)
	{
		packet << Packet::STRING;
		packet << arg;
	}
}


class Host
{
private:
	ID ids;
	std::map<ID, std::function<std::any(std::vector<std::any>)>> methods;
	std::map<ID, void*> interfaces;

public:
	Host();

	ID Register(std::function<std::any(std::vector<std::any>)> handler);
	void Unregister(ID id);

	void RegisterInterface(ID id, void* _interface);
	void UnregisterInterface(ID id);
	void* LookupInterface(ID id);

	std::any Handle(ID id, std::vector<std::any> args);

protected:
	void any_to_packet(std::any value, sf::Packet& packet);

	void get_values(sf::Packet& packet, std::vector<std::any>& values, size_t readStart = 0);

};


class Server : public Host
{
private:
	std::mutex lock;
	sf::TcpListener listener;
	sf::SocketSelector selector;
	std::thread *acceptor;
	std::vector<sf::TcpSocket*> clients;
	bool running;

public:
	Server();
	~Server() noexcept(false);

	void Listen(int port = 5000);

	void Run();
	void Stop();

private:
	void dispatch(sf::Packet& request, sf::Packet& reply);
};


class Client : public Host
{
private:
	sf::TcpSocket socket;

public:
	Client();

	void Connect(std::string host = "127.0.0.1", int port = 5000);
	
	template <typename... Args>
	std::vector<std::any> Call(ID method_id, Args&&... args)
	{
		sf::Packet request;

		request << method_id;

		(put_arg(request, std::forward<Args>(args)), ...);

		socket.send(request);


		sf::Packet reply;

		socket.receive(reply);

		std::vector<std::any> result;

		get_values(reply, result);

		return result;
	}


	template <typename... Args>
	std::vector<std::any> Call2(ID method_id, const void* ptr, size_t length, Args&&... args)
	{
		sf::Packet request;

		request << method_id;

		(put_arg(request, std::forward<Args>(args)), ...);

		request << Packet::DATA;
		request.append(ptr, length);

		socket.send(request);


		sf::Packet reply;

		socket.receive(reply);

		std::vector<std::any> result;

		get_values(reply, result);

		return result;
	}

};


class InterfaceClient
{
protected:
	Client& client;
	ID method_id;

public:
	typedef enum {
		RELEASE
	} Method;

protected:
	InterfaceClient(Client& client, ID method_id);
	~InterfaceClient();

public:
	ID GetMethodID();
};


}
