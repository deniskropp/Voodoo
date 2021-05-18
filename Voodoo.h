#pragma once

#include <assert.h>

#include <any>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <thread>

#include <SFML/Network.hpp>


namespace Voodoo {

/*
 * ID class used for methods, interfaces and cleanup handlers
 */
class ID
{
private:
	sf::Uint64 value;

public:
	ID();
	ID(sf::Uint64 value);

	sf::Uint64 operator *() const { return value; }

	bool operator < (const ID& other) const;

	operator bool() const
	{
		return value != 0;
	}
};


sf::Packet& operator <<(sf::Packet& packet, const ID& id);
sf::Packet& operator >>(sf::Packet& packet, ID& id);


/*
 * Packet class holding our value type definition
 */
class Packet
{
public:
	/*
	 * This defines the type of value for packet data
	 */
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
	/*
	 * Template function for data being appended to a packet
	 * 
	 * Specializations will write type and value accordingly
	 */
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


/*
 * Base class for client and server classes
 */
class Host
{
private:
	sf::Uint64 ids;
	std::map<ID, std::function<std::any(std::vector<std::any>)>> methods;
	std::map<ID, void*> interfaces;

public:
	Host();

	/*
	 * Generate a new ID for usage as method, interface or cleanup handler ID
	 */
	ID MakeID();

	/*
	 * Register method for incoming calls. Generates a new ID using MakeID.
	 */
	ID Register(std::function<std::any(std::vector<std::any>)> handler);
	void Unregister(ID id);

	/*
	 * Register interface for later lookup as a resource being passed to a method.
	 */
	void RegisterInterface(ID id, void* _interface);
	void UnregisterInterface(ID id);
	void* LookupInterface(ID id);

	/*
	 * Handle incoming call based on method ID.
	 */
	std::any Handle(ID id, std::vector<std::any> args);

protected:
	/*
	 * Append data to a packet.
	 */
	void any_to_packet(std::any value, sf::Packet& packet);

	/*
	 * Get data from a packet.
	 */
	void get_values(sf::Packet& packet, std::vector<std::any>& values, size_t readStart = 0);

};


/*
 * Server class for running the service on a TCP socket.
 */
class Server : public Host
{
private:
	std::mutex lock;
	sf::TcpListener listener;
	sf::SocketSelector selector;
	std::thread *acceptor;
	std::vector<sf::TcpSocket*> clients;
	bool running;
	static thread_local sf::TcpSocket* current_client;

	typedef std::function<void(void)> CleanupHandler;
	std::map<sf::TcpSocket*, std::map<ID,CleanupHandler>> cleanups;

public:
	Server();
	~Server() noexcept(false);

	/*
	 * Put server socket in listening mode and accept incoming connections in a thread.
	 */
	void Listen(int port = 5000);

	/*
	 * Handle incoming calls on any connection.
	 */
	void Run();

	/*
	 * Stop accepting new connections.
	 */
	void Stop();

	/*
	 * Register cleanup handler for the current client being handled.
	 */
	void PushCleanup(ID cleanup_id, CleanupHandler handler);
	void RemoveCleanup(ID cleanup_id);

private:
	/*
	 * Run cleanup handlers for the specified socket.
	 */
	void cleanup(sf::TcpSocket* socket);

private:
	/*
	 * Handle request (incoming call) and fill packet for reply.
	 */
	void dispatch(sf::Packet& request, sf::Packet& reply);
};


/*
 * Client class for using the service via TCP socket.
 */
class Client : public Host
{
private:
	sf::TcpSocket socket;

public:
	Client();

	/*
	 * Connect to server specified by host and port number.
	 */
	void Connect(std::string host = "127.0.0.1", int port = 5000);
	
	/*
	 * Make a call to the server and return the reply as a vector.
	 */
	template <typename... Args>
	std::vector<std::any> Call(ID method_id, Args&&... args)
	{
		sf::Packet request;

		request << method_id;

		/*
		 * Append all arguments to the request packet.
		 */
		(put_arg(request, std::forward<Args>(args)), ...);

		socket.send(request);


		/*
		 * Receive and parse the reply packet.
		 */
		sf::Packet reply;

		socket.receive(reply);

		std::vector<std::any> result;

		get_values(reply, result);

		return result;
	}

	/*
	 * Make a call to the server (with data buffer) and return the reply as a vector.
	 */
	template <typename... Args>
	std::vector<std::any> Call2(ID method_id, const void* ptr, size_t length, Args&&... args)
	{
		sf::Packet request;

		request << method_id;

		/*
		 * Append all arguments to the request packet.
		 */
		(put_arg(request, std::forward<Args>(args)), ...);

		/*
		 * Append data buffer to the request packet.
		 */
		request << Packet::DATA;
		request.append(ptr, length);

		socket.send(request);


		/*
		 * Receive and parse the reply packet.
		 */
		sf::Packet reply;

		socket.receive(reply);

		std::vector<std::any> result;

		get_values(reply, result);

		return result;
	}

};


/*
 * Interface helper on client side.
 */
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
	ID GetMethodID() const;
};


/*
 * Interface helper on server side.
 */
template <typename IFace>
class InterfaceServer
{
protected:
	Server& server;
	ID method_id;

protected:
	InterfaceServer(Server& server)
		:
		server(server)
	{
		method_id = server.Register([&server, this](std::vector<std::any> args) -> std::any
			{
				typename IFace::Method method = (typename IFace::Method)(std::any_cast<int>(args[0]));

				/*
				 * Common handler for releasing the interface.
				 */
				if (method == IFace::RELEASE) {
					server.RemoveCleanup(method_id);
					delete this;
					return 0;
				}

				/*
				 * Specific handlers for interface API.
				 */
				std::function<std::any(std::vector<std::any>)> handler = Lookup(method);

				return handler(args);
			});

		server.RegisterInterface(method_id, this);

		server.PushCleanup(method_id, [this]() {
				delete this;
			});
	}

	~InterfaceServer()
	{
		server.UnregisterInterface(method_id);
		server.Unregister(method_id);
	}

	/*
	 * This method has to be implemented by each server side interface class for handling incoming calls.
	 */
	virtual std::function<std::any(std::vector<std::any>)> Lookup(typename IFace::Method method) const = 0;

public:
	ID GetMethodID() const
	{
		return method_id;
	}
};


}
