#pragma once

#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

#include "Voodoo.h"


namespace VoodooTest {


class Setup
{
public:
	bool test_server;
	bool test_client;

	unsigned int mode;
	std::string host;

	Setup(Voodoo::Server &server,
		  Voodoo::Client &client)
		:
		test_server(false),
		test_client(false),
		mode(0),
		host("127.0.0.1")
	{
		/* Select Server/Client Mode */

		std::cout << "Please select running" << std::endl;
		std::cout << " 1         both client & server (127.0.0.1:5000)" << std::endl;
		std::cout << " 2         server only (0.0.0.0:5000)" << std::endl;
		std::cout << " 3         local client (127.0.0.1:5000)" << std::endl;
		std::cout << " 4 <host>  remote client (host:5000)" << std::endl;
		std::cout << std::endl;

		std::cout << "mode> ";
		std::cin >> mode;

		switch (mode) {
		case 1:
			test_server = true;
			test_client = true;
			break;
		case 2:
			test_server = true;
			break;
		case 3:
			test_client = true;
			break;
		case 4:
			test_client = true;
			std::cout << "host> ";
			std::cin >> host;
			break;
		default:
			throw std::runtime_error("invalid mode selected");
		}

		/* Initialize Server(Listen) and Client(Connect) */

		if (test_server)
			server.Listen();

		if (test_client) {
			try {
				std::cout << "Connecting to " << host << "...";

				client.Connect(host);
			}
			catch (...) {
				std::cout << " FAILED!" << std::endl;
				throw;
			}

			std::cout << std::endl;
		}
	}

	void wait_server()
	{
		/* Read from console in Server only mode */

		char c[2];

		std::cout << "server running, stop by hitting return> ";

		std::cin.read(c, 2);
	}
};


}
