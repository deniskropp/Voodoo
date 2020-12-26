#include <iostream>

#include <SFML/Graphics.hpp>

#include "Voodoo.h"



class IVoodooGraphics : public Voodoo::InterfaceClient
{
public:
	typedef enum {
		FILL_RECTANGLE = RELEASE + 1,
		DRAW_SPRITE,
		DRAW_SPRITE_SCALED,
		TEXTURE_TRIANGLE,
		DRAW_TEXT,
		FLIP_DISPLAY,
		CREATE_IMAGE,
		CREATE_TEXTURE,
		CREATE_FONT,
		GET_EVENT
	} Method;

public:
	IVoodooGraphics(Voodoo::Client& client, Voodoo::ID method_id)
		:
		InterfaceClient(client, method_id)
	{
	}

	void FillRectangle( sf::Vector2f pos, sf::Vector2f size, sf::Color color )
	{
		client.Call(method_id, (int)FILL_RECTANGLE, pos.x, pos.y, size.x, size.y, color.r, color.g, color.b, color.a );
	}

	void DrawSprite(sf::Vector2f pos, InterfaceClient* texture)
	{
		client.Call(method_id, (int)DRAW_SPRITE, pos.x, pos.y, texture->GetMethodID());
	}

	void DrawSpriteScaled(sf::Vector2f pos, sf::Vector2f size, InterfaceClient* texture)
	{
		client.Call(method_id, (int)DRAW_SPRITE_SCALED, pos.x, pos.y, size.x, size.y, texture->GetMethodID());
	}

	class Triangle
	{
	public:
		sf::Vector2f p1;
		sf::Vector2f t1;
		sf::Vector2f p2;
		sf::Vector2f t2;
		sf::Vector2f p3;
		sf::Vector2f t3;
	};

	void TextureTriangle(const Triangle &triangle, InterfaceClient* texture)
	{
		client.Call(method_id, (int)TEXTURE_TRIANGLE,
									triangle.p1.x, triangle.p1.y,
									triangle.t1.x, triangle.t1.y,
									triangle.p2.x, triangle.p2.y,
									triangle.t2.x, triangle.t2.y,
									triangle.p3.x, triangle.p3.y,
									triangle.t3.x, triangle.t3.y,
									texture->GetMethodID());
	}

	void DrawText(sf::Vector2f pos, InterfaceClient* font, int characterSize, std::string text, sf::Color color)
	{
		client.Call(method_id, (int)DRAW_TEXT, pos.x, pos.y, font->GetMethodID(), characterSize, text, color.r, color.g, color.b, color.a);
	}

	void FlipDisplay()
	{
		client.Call(method_id, (int)FLIP_DISPLAY);
	}

	Voodoo::ID CreateImage(int width, int height)
	{
		auto result = client.Call(method_id, (int)CREATE_IMAGE, width, height);

		return std::any_cast<Voodoo::ID>(result[0]);
	}

	Voodoo::ID CreateTexture(InterfaceClient *image)
	{
		auto result = client.Call(method_id, (int)CREATE_TEXTURE, image->GetMethodID());

		return std::any_cast<Voodoo::ID>(result[0]);
	}

	Voodoo::ID CreateFont()
	{
		auto result = client.Call(method_id, (int)CREATE_FONT);

		return std::any_cast<Voodoo::ID>(result[0]);
	}

public:
	class Event
	{
	public:
		enum class Type {
			None,
			WindowClosed,
			KeyPressed,
			KeyReleased,
			ButtonPressed,
			ButtonReleased,
			Motion,
			Wheel
		};

		typedef sf::Keyboard::Key Key;
		typedef sf::Mouse::Button Button;

		Type   type;
		Key    key;		// KeyPressed, KeyReleased
		Button button;	// ButtonPressed, ButtonReleased
		int    x;		// Motion, Wheel (horizontal)
		int    y;		// Motion, Wheel (vertical)
	};

	bool GetEvent(Event& ev)
	{
		auto result = client.Call(method_id, (int)GET_EVENT);

		ev.type = (Event::Type)std::any_cast<int>(result[0]);

		switch (ev.type) {
		case Event::Type::None:
			break;
		case Event::Type::WindowClosed:
			break;
		case Event::Type::KeyPressed:
		case Event::Type::KeyReleased:
			ev.key = (Event::Key)std::any_cast<int>(result[1]);
			break;
		case Event::Type::ButtonPressed:
		case Event::Type::ButtonReleased:
			ev.button = (Event::Button)std::any_cast<int>(result[1]);
			break;
		case Event::Type::Motion:
		case Event::Type::Wheel:
			ev.x = std::any_cast<int>(result[1]);
			ev.y = std::any_cast<int>(result[2]);
			break;
		}

		return ev.type != Event::Type::None;
	}
};

class IVoodooImage : public Voodoo::InterfaceClient
{
public:
	typedef enum {
		WRITE = RELEASE + 1,
		LOAD
	} Method;

public:
	IVoodooImage(Voodoo::Client& client, Voodoo::ID method_id)
		:
		InterfaceClient(client, method_id)
	{
	}

	void Write(sf::IntRect rect, const void *data, int pitch)
	{
		for (int y=0; y<rect.height; y++)
			client.Call2(method_id, (const char*)data + pitch * y, rect.width * 4, (int)WRITE, rect.left, rect.top + y, rect.width);
	}

	void LoadFromFile(std::string filename)
	{
		FILE* f;

		if (fopen_s(&f, filename.c_str(), "r"))
			throw std::runtime_error(filename);

		fseek(f, 0, SEEK_END);

		long size = ftell(f);
		void *buf = malloc(size);

		fseek(f, 0, SEEK_SET);

		fread(buf, size, 1, f);

		//std::cout << "size " << size << "  " << (int)((const char*)buf)[0] << "  " << (int)((const char*)buf)[1] << std::endl;
		std::cout << "size " << size << "  " << (const char*)buf << std::endl;

		client.Call2(method_id, buf, size, (int)LOAD, (int)size);

		fclose(f);

		free(buf);
	}
};

class IVoodooTexture : public Voodoo::InterfaceClient
{
public:
	typedef enum {
		DUMMY = RELEASE + 1
	} Method;

public:
	IVoodooTexture(Voodoo::Client& client, Voodoo::ID method_id)
		:
		InterfaceClient(client, method_id)
	{
	}
};

class IVoodooFont : public Voodoo::InterfaceClient
{
public:
	typedef enum {
		LOAD = RELEASE + 1
	} Method;

public:
	IVoodooFont(Voodoo::Client& client, Voodoo::ID method_id)
		:
		InterfaceClient(client, method_id)
	{
	}

	void LoadFromFile(std::string filename)
	{
		FILE* f;

		if (fopen_s(&f, filename.c_str(), "r"))
			throw std::runtime_error(filename);

		fseek(f, 0, SEEK_END);

		long size = ftell(f);
		void* buf = malloc(size);

		fseek(f, 0, SEEK_SET);

		fread(buf, size, 1, f);

		//std::cout << "size " << size << "  " << (int)((const char*)buf)[0] << "  " << (int)((const char*)buf)[1] << std::endl;
		std::cout << "size " << size << "  " << (const char*)buf << std::endl;

		client.Call2(method_id, buf, size, (int)LOAD, (int)size);

		fclose(f);

		free(buf);
	}
};



class IVoodooImage_Server
{
private:
	Voodoo::Server& server;
	Voodoo::ID method_id;
	sf::Image image;

public:
	IVoodooImage_Server(Voodoo::Server& server, int width, int height)
		:
		server(server)
	{
		image.create(width, height);

		method_id = server.Register([&server, this](std::vector<std::any> args)
			{
				std::any ret = 0;

				int method = std::any_cast<int>(args[0]);

				switch (method) {
				case IVoodooImage::RELEASE:
					delete this;
					break;
				case IVoodooImage::WRITE:
					write_image(std::any_cast<int>(args[1]),
						        std::any_cast<int>(args[2]),
						        std::any_cast<int>(args[3]),
						        std::any_cast<const void*>(args[4]));
					break;
				case IVoodooImage::LOAD:
					//std::cout << (const char*)std::any_cast<const void*>(args[2]) << std::endl;
					image.loadFromMemory(std::any_cast<const void*>(args[2]), std::any_cast<int>(args[1]));
					//image.loadFromFile("bitmap.png");
					break;
				}

				return ret;
			});

		server.RegisterInterface( method_id, this );
	}

	~IVoodooImage_Server()
	{
		server.UnregisterInterface(method_id);
		server.Unregister(method_id);
	}

	Voodoo::ID GetMethodID() const
	{
		return method_id;
	}

	sf::Image& GetImage()
	{
		return image;
	}

private:
	void write_image(int x, int y, int width, const void* data)
	{
		sf::Image src;
		
		src.create(width, 1, (const sf::Uint8*) data);

		image.copy(src, x, y);
	}
};

class IVoodooTexture_Server
{
private:
	Voodoo::Server& server;
	Voodoo::ID method_id;
	sf::Texture texture;

public:
	IVoodooTexture_Server(Voodoo::Server& server, IVoodooImage_Server* image)
		:
		server(server)
	{
		texture.loadFromImage(image->GetImage());

		method_id = server.Register([&server, this](std::vector<std::any> args)
			{
				std::any ret = 0;

				int method = std::any_cast<int>(args[0]);

				switch (method) {
				case IVoodooTexture::RELEASE:
					delete this;
					break;
				}

				return ret;
			});

		server.RegisterInterface(method_id, this);
	}

	~IVoodooTexture_Server()
	{
		server.UnregisterInterface(method_id);
		server.Unregister(method_id);
	}

	Voodoo::ID GetMethodID() const
	{
		return method_id;
	}

	sf::Texture& GetTexture()
	{
		return texture;
	}
};

class IVoodooFont_Server
{
private:
	Voodoo::Server& server;
	Voodoo::ID method_id;
	sf::Font font;

public:
	IVoodooFont_Server(Voodoo::Server& server)
		:
		server(server)
	{
		method_id = server.Register([&server, this](std::vector<std::any> args)
			{
				std::any ret = 0;

				int method = std::any_cast<int>(args[0]);

				switch (method) {
				case IVoodooFont::RELEASE:
					delete this;
					break;
				case IVoodooFont::LOAD:
					//std::cout << (const char*)std::any_cast<const void*>(args[2]) << std::endl;
					//font.loadFromMemory(std::any_cast<const void*>(args[2]), std::any_cast<int>(args[1]));
					font.loadFromFile("FreeSans.ttf");
					break;
				}

				return ret;
			});

		server.RegisterInterface(method_id, this);
	}

	~IVoodooFont_Server()
	{
		server.UnregisterInterface(method_id);
		server.Unregister(method_id);
	}

	Voodoo::ID GetMethodID() const
	{
		return method_id;
	}

	sf::Font& GetFont()
	{
		return font;
	}
};


class IVoodooGraphics_Server
{
private:
	Voodoo::Server& server;
	Voodoo::ID method_id;
	sf::RenderWindow window;

public:
	IVoodooGraphics_Server(Voodoo::Server& server)
		:
		server(server),
		window( sf::VideoMode(1024, 768), "Voodoo Graphics" )
	{
		method_id = server.Register([&server, this](std::vector<std::any> args)
			{
				std::any ret = 0;

				int method = std::any_cast<int>(args[0]);

				switch (method) {
				case IVoodooGraphics::RELEASE:
					delete this;
					break;
				case IVoodooGraphics::FILL_RECTANGLE:
					fill_rectangle(args);
					break;
				case IVoodooGraphics::DRAW_SPRITE:
					draw_sprite(args);
					break;
				case IVoodooGraphics::DRAW_SPRITE_SCALED:
					draw_sprite_scaled(args);
					break;
				case IVoodooGraphics::TEXTURE_TRIANGLE:
					texture_triangle(args);
					break;
				case IVoodooGraphics::DRAW_TEXT:
					draw_text(args);
					break;
				case IVoodooGraphics::FLIP_DISPLAY:
					flip_display();
					break;
				case IVoodooGraphics::CREATE_IMAGE: {
					auto image = new IVoodooImage_Server(server, std::any_cast<int>(args[1]), std::any_cast<int>(args[2]));

					ret = image->GetMethodID();
					break;
					}
				case IVoodooGraphics::CREATE_TEXTURE: {
					auto texture = new IVoodooTexture_Server(server, (IVoodooImage_Server*)server.LookupInterface(std::any_cast<Voodoo::ID>(args[1])));

					ret = texture->GetMethodID();
					break;
					}
				case IVoodooGraphics::CREATE_FONT: {
					auto font = new IVoodooFont_Server(server);

					ret = font->GetMethodID();
					break;
					}
				case IVoodooGraphics::GET_EVENT:
					ret = get_event();
					break;
				}

				return ret;
			});
	}

	~IVoodooGraphics_Server()
	{
		server.Unregister(method_id);
	}

	Voodoo::ID GetMethodID() const
	{
		return method_id;
	}

private:
	void fill_rectangle(std::vector<std::any> args)
	{
		sf::Vector2f pos;
		sf::Vector2f size;
		sf::Color color;

		pos.x = std::any_cast<float>(args[1]);
		pos.y = std::any_cast<float>(args[2]);

		size.x = std::any_cast<float>(args[3]);
		size.y = std::any_cast<float>(args[4]);

		color.r = std::any_cast<sf::Uint8>(args[5]);
		color.g = std::any_cast<sf::Uint8>(args[6]);
		color.b = std::any_cast<sf::Uint8>(args[7]);
		color.a = std::any_cast<sf::Uint8>(args[8]);

		sf::RectangleShape rect;

		rect.setPosition(pos);
		rect.setSize(size);
		rect.setFillColor(color);

		window.draw(rect);
	}

	void draw_sprite(std::vector<std::any> args)
	{
		sf::Vector2f pos;
		IVoodooTexture_Server* texture = (IVoodooTexture_Server*)server.LookupInterface(std::any_cast<Voodoo::ID>(args[3]));

		pos.x = std::any_cast<float>(args[1]);
		pos.y = std::any_cast<float>(args[2]);


		sf::Sprite sprite;

		sprite.setTexture(texture->GetTexture());
		sprite.setPosition(pos);

		window.draw(sprite);
	}

	void draw_sprite_scaled(std::vector<std::any> args)
	{
		sf::Vector2f pos, size;
		IVoodooTexture_Server* texture = (IVoodooTexture_Server*)server.LookupInterface(std::any_cast<Voodoo::ID>(args[5]));

		pos.x = std::any_cast<float>(args[1]);
		pos.y = std::any_cast<float>(args[2]);

		size.x = std::any_cast<float>(args[3]);
		size.y = std::any_cast<float>(args[4]);


		sf::Sprite sprite;

		sprite.setTexture(texture->GetTexture());
		sprite.setPosition(pos);

		window.draw(sprite);
	}

	void texture_triangle(std::vector<std::any> args)
	{
		IVoodooTexture_Server* texture = (IVoodooTexture_Server*)server.LookupInterface(std::any_cast<Voodoo::ID>(args[13]));

		sf::VertexArray vertices(sf::Triangles, 3);

		vertices[0].position.x = std::any_cast<float>(args[1]);
		vertices[0].position.y = std::any_cast<float>(args[2]);
		vertices[0].texCoords.x = std::any_cast<float>(args[3]);
		vertices[0].texCoords.y = std::any_cast<float>(args[4]);
		vertices[1].position.x = std::any_cast<float>(args[5]);
		vertices[1].position.y = std::any_cast<float>(args[6]);
		vertices[1].texCoords.x = std::any_cast<float>(args[7]);
		vertices[1].texCoords.y = std::any_cast<float>(args[8]);
		vertices[2].position.x = std::any_cast<float>(args[9]);
		vertices[2].position.y = std::any_cast<float>(args[10]);
		vertices[2].texCoords.x = std::any_cast<float>(args[11]);
		vertices[2].texCoords.y = std::any_cast<float>(args[12]);

		sf::RenderStates states = sf::RenderStates::Default;

		states.texture = &texture->GetTexture();
		
		window.draw(&vertices[0], 3, sf::Triangles, states);
	}

	void draw_text(std::vector<std::any> args)
	{
		IVoodooFont_Server* font = (IVoodooFont_Server*)server.LookupInterface(std::any_cast<Voodoo::ID>(args[3]));

		sf::Text text;

		text.setFont(font->GetFont());
		text.setPosition(sf::Vector2f(std::any_cast<float>(args[1]), std::any_cast<float>(args[2])));
		text.setCharacterSize(std::any_cast<int>(args[4]));
		text.setString(std::any_cast<std::string>(args[5]));
		text.setFillColor(sf::Color(std::any_cast<sf::Uint8>(args[6]),
									std::any_cast<sf::Uint8>(args[7]),
									std::any_cast<sf::Uint8>(args[8]),
									std::any_cast<sf::Uint8>(args[9])));

		//std::cout << "Drawing text: " << std::any_cast<std::string>(args[5]) << std::endl;

		window.draw(text);
	}

	void flip_display()
	{
		window.display();

		window.clear();
	}

	std::vector<std::any> get_event()
	{
		std::vector<std::any> ret;

		sf::Event event;

		if (window.pollEvent(event)) {
			switch (event.type) {
			case sf::Event::Closed:
				window.close();
				ret.push_back((int)IVoodooGraphics::Event::Type::WindowClosed);
				break;
			case sf::Event::KeyPressed:
				ret.push_back((int)IVoodooGraphics::Event::Type::KeyPressed);
				ret.push_back((int)event.key.code);
				break;
			case sf::Event::KeyReleased:
				ret.push_back((int)IVoodooGraphics::Event::Type::KeyReleased);
				ret.push_back((int)event.key.code);
				break;
			case sf::Event::MouseButtonPressed:
				ret.push_back((int)IVoodooGraphics::Event::Type::ButtonPressed);
				ret.push_back((int)event.mouseButton.button);
				break;
			case sf::Event::MouseButtonReleased:
				ret.push_back((int)IVoodooGraphics::Event::Type::ButtonReleased);
				ret.push_back((int)event.mouseButton.button);
				break;
			case sf::Event::MouseMoved:
				ret.push_back((int)IVoodooGraphics::Event::Type::Motion);
				ret.push_back(event.mouseMove.x);
				ret.push_back(event.mouseMove.y);
				break;
			default:
				ret.push_back((int)IVoodooGraphics::Event::Type::None);
				break;
			}
		}
		else
			ret.push_back((int)IVoodooGraphics::Event::Type::None);

		return ret;
	}
};



int main()
{
	Voodoo::Server server;
	Voodoo::Client client;

	Voodoo::ID graphics_id = server.Register([&server](std::vector<std::any> args)
		{
			auto graphics = new IVoodooGraphics_Server(server);

			return graphics->GetMethodID();
		});

	std::thread server_loop([&server]() { server.Run(); });


	auto result = client.Call(graphics_id);

	auto graphics = new IVoodooGraphics(client, std::any_cast<Voodoo::ID>(result[0]));


	sf::Image img;

	img.loadFromFile("bitmap.png");
	img.createMaskFromColor(sf::Color::Black);

	auto image = new IVoodooImage(client, graphics->CreateImage(img.getSize().x, img.getSize().y));

#if 0
	sf::Uint8 data[400 * 100];

	memset(data, 0x77, 400 * 100);

	image->Write(sf::IntRect(0, 0, 100, 100), data, 400);
#else
	image->Write(sf::IntRect(0, 0, img.getSize().x, img.getSize().y), img.getPixelsPtr(), img.getSize().x * 4);
#endif

	auto texture = new IVoodooTexture(client, graphics->CreateTexture(image));

	auto font = new IVoodooFont(client, graphics->CreateFont());

	font->LoadFromFile("FreeSans.ttf");


	std::vector<sf::Vector2f> points;

	bool windowClosed = false;

	while (!windowClosed) {
		graphics->FillRectangle(sf::Vector2f(100, 100), sf::Vector2f(400, 300), sf::Color(255, 0, 0, 255));
		graphics->FillRectangle(sf::Vector2f(300, 150), sf::Vector2f(400, 300), sf::Color(0, 0, 255, 255));
		graphics->FillRectangle(sf::Vector2f(150, 300), sf::Vector2f(400, 300), sf::Color(100, 100, 100, 255));

		graphics->DrawSprite(sf::Vector2f(210, 210), texture);
		graphics->DrawSprite(sf::Vector2f(400, 300), texture);
		graphics->DrawSprite(sf::Vector2f(240, 450), texture);

		for (auto p : points)
			graphics->FillRectangle(p, sf::Vector2f(10, 10), sf::Color(100, 255, 100, 255));


		IVoodooGraphics::Triangle triangle;

		triangle.p1.x = 10;
		triangle.p1.y = 10;
		triangle.t1.x = 0;
		triangle.t1.y = 0;
		triangle.p2.x = 200;
		triangle.p2.y = 10;
		triangle.t2.x = 500;
		triangle.t2.y = 0;
		triangle.p3.x = 10;
		triangle.p3.y = 200;
		triangle.t3.x = 0;
		triangle.t3.y = 500;

		graphics->TextureTriangle(triangle, texture);

		graphics->DrawText(sf::Vector2f(100, 100), font, 23, "Text Example", sf::Color(230, 230, 230, 255));
		graphics->DrawText(sf::Vector2f(150, 130), font, 30, "Another Text Example", sf::Color(250, 250, 250, 255));

		graphics->FlipDisplay();


		IVoodooGraphics::Event event;

		while (graphics->GetEvent(event)) {
			switch (event.type) {
			case IVoodooGraphics::Event::Type::WindowClosed:
				windowClosed = true;
				break;
			case IVoodooGraphics::Event::Type::KeyPressed:
				switch (event.key) {
				case IVoodooGraphics::Event::Key::Escape:
					windowClosed = true;
					break;
				default:
					break;
				}
				break;
			case IVoodooGraphics::Event::Type::ButtonPressed:
				points.push_back(sf::Vector2f((float)event.x, (float)event.y));
				break;
			default:
				break;
			}
		}
	}
	
	delete font;
	delete texture;
	delete image;
	delete graphics;


	server.Stop();

	server_loop.join();

	return 0;
}