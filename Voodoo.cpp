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

ID Host::Register(std::function<std::any(std::vector<std::any>)> handler)
{
	ID id( ++ids.value );

	if (id.value == 0)
		throw std::runtime_error("out of id space");

	methods[id] = handler;

	return id;
}

void Host::Unregister(ID id)
{
	auto it = methods.find(id);

	if (it == methods.end())
		throw std::runtime_error("invalid id");

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
		throw std::runtime_error("invalid id");

	interfaces.erase(it);
}

void* Host::LookupInterface(ID id)
{
	auto it = interfaces.find(id);

	if (it == interfaces.end())
		throw std::runtime_error("invalid id");

	return it->second;
}

std::any Host::Handle(ID id, std::vector<std::any> args)
{
	auto it = methods.find(id);

	if (it == methods.end())
		throw std::runtime_error("invalid id");

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

void Host::get_values(sf::Packet& packet, std::vector<std::any>& values)
{
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

		switch (t) {
		case Packet::ID:
			packet >> id;
			values.push_back(id);
			break;
		case Packet::INT8:
			packet >> i8;
			values.push_back(i8);
			break;
		case Packet::UINT8:
			packet >> u8;
			values.push_back(u8);
			break;
		case Packet::INT16:
			packet >> i16;
			values.push_back(i16);
			break;
		case Packet::UINT16:
			packet >> u16;
			values.push_back(u16);
			break;
		case Packet::INT32:
			packet >> i32;
			values.push_back(i32);
			break;
		case Packet::UINT32:
			packet >> u32;
			values.push_back(u32);
			break;
		case Packet::INT64:
			packet >> i64;
			values.push_back(i64);
			break;
		case Packet::UINT64:
			packet >> u64;
			values.push_back(u64);
			break;
		case Packet::FLOAT32:
			packet >> f32;
			values.push_back(f32);
			break;
		case Packet::FLOAT64:
			packet >> f64;
			values.push_back(f64);
			break;
		case Packet::STRING:
			packet >> str;
			values.push_back(str);
			break;
		case Packet::DATA:
			ptr = (const char*)packet.getData() + packet.getReadPosition();
			values.push_back(ptr);
			return;
		default:
			throw std::runtime_error("unknown/unimplemented type");
		}
	}
}


Server::Server()
	:
	running(true)
{
	if (listener.listen(5000) != sf::Socket::Done)
		throw std::runtime_error("could not listen");

	acceptor = new std::thread([this] () {
			while (running) {
				sf::TcpSocket *socket = new sf::TcpSocket();

				if (listener.accept(*socket) == sf::Socket::Done) {
					clients.push_back(socket);

					selector.add(*socket);
				}
				else
					delete socket;
			}
		});
}

Server::~Server()
{
	assert(!running);

	listener.close();

	acceptor->join();

	delete acceptor;

	for (auto socket : clients)
		delete socket;
}

void Server::Run()
{
	while (running) {
		if (selector.wait(sf::milliseconds(50))) {
			for (auto socket : clients) {
				if (selector.isReady(*socket)) {
					sf::Packet request, reply;

					socket->receive(request);

					dispatch(request, reply);

					socket->send(reply);
				}
			}
		}
	}
}

void Server::Stop()
{
	running = false;
}

void Server::dispatch(sf::Packet& request, sf::Packet &reply)
{
	ID method_id;
	std::vector<std::any> args;

	request >> method_id;

	get_values(request, args);

	std::any result = Handle(method_id, args);

	any_to_packet(result, reply);
}


Client::Client()
{
	sf::Socket::Status status = socket.connect("127.0.0.1", 5000);

	if (status != sf::Socket::Done)
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

ID InterfaceClient::GetMethodID()
{
	return method_id;
}


}
