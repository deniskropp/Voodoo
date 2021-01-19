#include "Voodoo.h"


namespace Voodoo {

ID::ID()
	:
	value(0)
{
}

ID::ID(sf::Uint64 value)
	:
	value(value)
{
}

bool ID::operator < (const ID& other) const
{
	return value < other.value;
}


sf::Packet& operator <<(sf::Packet& packet, const ID& id)
{
	return packet << id.value;
}

sf::Packet& operator >>(sf::Packet& packet, ID& id)
{
	return packet >> id.value;
}


Host::Host()
{
}

ID Host::MakeID()
{
	ID id(++ids.value);

	if (id.value == 0)
		throw std::runtime_error("out of id space");

	return id;
}

ID Host::Register(std::function<std::any(std::vector<std::any>)> handler)
{
	ID id = MakeID();

	methods[id] = handler;

	return id;
}

void Host::Unregister(ID id)
{
	auto it = methods.find(id);

	if (it == methods.end())
		throw std::runtime_error(std::string("invalid method id ") + std::to_string(*id));

	methods.erase(it);
}

void Host::RegisterInterface(ID id, void *_interface)
{
	interfaces[id] = _interface;
}

void Host::UnregisterInterface(ID id)
{
	auto it = interfaces.find(id);

	if (it == interfaces.end())
		throw std::runtime_error(std::string("invalid interface id ") + std::to_string(*id));

	interfaces.erase(it);
}

void* Host::LookupInterface(ID id)
{
	auto it = interfaces.find(id);

	if (it == interfaces.end())
		throw std::runtime_error(std::string("invalid interface id ") + std::to_string(*id));

	return it->second;
}

std::any Host::Handle(ID id, std::vector<std::any> args)
{
	auto it = methods.find(id);

	if (it == methods.end())
		throw std::runtime_error(std::string("invalid method id ") + std::to_string(*id));

	return it->second(args);
}

void Host::any_to_packet(std::any value, sf::Packet& packet)
{
	if (value.type() == typeid(ID)) {
		packet << Packet::ID;
		packet << std::any_cast<ID>(value);
	}
	else if (value.type() == typeid(char)) {
		packet << Packet::INT8;
		packet << std::any_cast<char>(value);
	}
	else if (value.type() == typeid(unsigned char)) {
		packet << Packet::UINT8;
		packet << std::any_cast<unsigned char>(value);
	}
	else if (value.type() == typeid(short)) {
		packet << Packet::INT16;
		packet << std::any_cast<short>(value);
	}
	else if (value.type() == typeid(unsigned short)) {
		packet << Packet::UINT16;
		packet << std::any_cast<unsigned short>(value);
	}
	else if (value.type() == typeid(int)) {
		packet << Packet::INT32;
		packet << std::any_cast<int>(value);
	}
	else if (value.type() == typeid(unsigned int)) {
		packet << Packet::UINT32;
		packet << std::any_cast<unsigned int>(value);
	}
	else if (value.type() == typeid(long long)) {
		packet << Packet::INT64;
		packet << std::any_cast<long long>(value);
	}
	else if (value.type() == typeid(unsigned long long)) {
		packet << Packet::UINT64;
		packet << std::any_cast<unsigned long long>(value);
	}
	else if (value.type() == typeid(float)) {
		packet << Packet::FLOAT32;
		packet << std::any_cast<float>(value);
	}
	else if (value.type() == typeid(double)) {
		packet << Packet::FLOAT64;
		packet << std::any_cast<double>(value);
	}
	else if (value.type() == typeid(std::string)) {
		packet << Packet::STRING;
		packet << std::any_cast<std::string>(value);
	}
	else if (value.type() == typeid(const char*)) {
		packet << Packet::STRING;
		packet << std::any_cast<const char*>(value);
	}
	else if (value.type() == typeid(std::pair<const void*, size_t>)) {
		packet << Packet::DATA;
		packet.append(std::any_cast<std::pair<const void*, size_t>>(value).first, std::any_cast<std::pair<const void*, size_t>>(value).second);
	}
	else if (value.type() == typeid(std::vector<std::any>)) {
		auto values = std::any_cast<std::vector<std::any>>(value);

		for (auto v : values)
			any_to_packet(v, packet);
	}
	else
		throw std::runtime_error("unknown/unimplemented type");
}

void Host::get_values(sf::Packet& packet, std::vector<std::any>& values, size_t readStart)
{
	size_t readPosition = readStart;

	while (!packet.endOfPacket()) {
		int t;
		ID id;
		sf::Int8 i8;
		sf::Uint8 u8;
		sf::Int16 i16;
		sf::Uint16 u16;
		sf::Int32 i32;
		sf::Uint32 u32;
		sf::Int64 i64;
		sf::Uint64 u64;
		float f32;
		double f64;
		std::string str;
		const void* ptr;

		packet >> t;
		readPosition += sizeof(t);

		switch (t) {
		case Packet::ID:
			packet >> id;
			readPosition += sizeof(id);
			values.push_back(id);
			break;
		case Packet::INT8:
			packet >> i8;
			readPosition += sizeof(i8);
			values.push_back(i8);
			break;
		case Packet::UINT8:
			packet >> u8;
			readPosition += sizeof(u8);
			values.push_back(u8);
			break;
		case Packet::INT16:
			packet >> i16;
			readPosition += sizeof(i16);
			values.push_back(i16);
			break;
		case Packet::UINT16:
			packet >> u16;
			readPosition += sizeof(u16);
			values.push_back(u16);
			break;
		case Packet::INT32:
			packet >> i32;
			readPosition += sizeof(i32);
			values.push_back(i32);
			break;
		case Packet::UINT32:
			packet >> u32;
			readPosition += sizeof(u32);
			values.push_back(u32);
			break;
		case Packet::INT64:
			packet >> i64;
			readPosition += sizeof(i64);
			values.push_back(i64);
			break;
		case Packet::UINT64:
			packet >> u64;
			readPosition += sizeof(u64);
			values.push_back(u64);
			break;
		case Packet::FLOAT32:
			packet >> f32;
			readPosition += sizeof(f32);
			values.push_back(f32);
			break;
		case Packet::FLOAT64:
			packet >> f64;
			readPosition += sizeof(f64);
			values.push_back(f64);
			break;
		case Packet::STRING:
			packet >> str;
			readPosition += str.size() + 1;
			values.push_back(str);
			break;
		case Packet::DATA:
			ptr = (const char*)packet.getData() + readPosition;
			values.push_back(ptr);
			return;
		default:
			throw std::runtime_error("unknown/unimplemented type");
		}
	}
}


thread_local sf::TcpSocket* Server::current_client;

Server::Server()
	:
	running(false),
	acceptor(0)
{
}

Server::~Server() noexcept(false)
{
	std::unique_lock<std::mutex> l(lock);

	if (running)
		throw std::runtime_error("server not stopped before destruction");

	int localPort = listener.getLocalPort();

	if (localPort) {
		listener.close();

		sf::TcpSocket* hack = new sf::TcpSocket();
		hack->connect("127.0.0.1", localPort);
		delete hack;
	}

	l.unlock();

	if (acceptor) {
		acceptor->join();

		delete acceptor;
	}

	//??	l.lock();

	for (auto socket : clients)
		delete socket;
}

void Server::Listen(int port)
{
	if (acceptor)
		throw std::runtime_error("server already listening");

	if (listener.listen(port) != sf::Socket::Done)
		throw std::runtime_error("could not listen");

	running = true;

	acceptor = new std::thread([this] () {
			std::unique_lock<std::mutex> l(lock);

			while (running) {
				l.unlock();

				sf::TcpSocket *socket = new sf::TcpSocket();

				if (listener.accept(*socket) == sf::Socket::Done) {
					l.lock();

					clients.push_back(socket);

					selector.add(*socket);
				}
				else {
					delete socket;

					l.lock();
				}
			}
		});
}

void Server::Run()
{
	std::unique_lock<std::mutex> l(lock);

	while (running) {
		l.unlock();

		if (selector.wait(sf::milliseconds(50))) {
			l.lock();

			for (auto it = clients.begin(); it != clients.end(); it++) {
				auto socket = *it;

				if (selector.isReady(*socket)) {
					sf::Packet request, reply;

					if (socket->receive(request) != sf::Socket::Done) {
						cleanup(socket);

						clients.erase(it);
						break;
					}

					current_client = socket;

					dispatch(request, reply);

					current_client = NULL;
					
					socket->send(reply);
				}
			}

			l.unlock();
		}

		l.lock();
	}
}

void Server::Stop()
{
	std::unique_lock<std::mutex> l(lock);

	running = false;
}

void Server::PushCleanup(Voodoo::ID cleanup_id, CleanupHandler handler)
{
	if (!current_client)
		throw std::runtime_error("no current client");

	cleanups[current_client].insert(std::make_pair(cleanup_id, handler));
}

void Server::RemoveCleanup(Voodoo::ID cleanup_id)
{
	if (!current_client)
		throw std::runtime_error("no current client");

	cleanups[current_client].erase(cleanup_id);
}

void Server::cleanup(sf::TcpSocket* socket)
{
	auto it = cleanups.find(socket);

	if (it != cleanups.end()) {
		for (auto it2 = it->second.rbegin(); it2 != it->second.rend(); it2++)
			it2->second();

		cleanups.erase(it);
	}
}

void Server::dispatch(sf::Packet& request, sf::Packet &reply)
{
	ID method_id;

	if (request >> method_id) {
		std::vector<std::any> args;

		get_values(request, args, sizeof(ID));

		std::any result = Handle(method_id, args);

		any_to_packet(result, reply);
	}
}


Client::Client()
{
}

void Client::Connect(std::string host, int port)
{
	if (socket.getRemotePort() != 0)
		throw std::runtime_error("client already connected");

	if (socket.connect(host, port) != sf::Socket::Done)
		throw std::runtime_error("could not connect");
}


InterfaceClient::InterfaceClient(Client& client, ID method_id)
	:
	client(client),
	method_id(method_id)
{
}

InterfaceClient::~InterfaceClient()
{
	client.Call(method_id, (int)RELEASE);
}

ID InterfaceClient::GetMethodID() const
{
	return method_id;
}


}
