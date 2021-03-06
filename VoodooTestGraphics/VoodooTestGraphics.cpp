#include <array>
#include <iostream>

#include <SFML/Graphics.hpp>

#include <log.hpp>
#include <system.hpp>

#include "Voodoo.h"
#include "VoodooTest.h"



class IVoodooGraphics : public Voodoo::InterfaceClient
{
public:
	using Method = enum {
		RELEASE,

		FILL_RECTANGLE,
		DRAW_SPRITE,
		DRAW_SPRITE_SCALED,
		TEXTURE_TRIANGLE,
		DRAW_TEXT,
		RENDER_VERTEXARRAY,
		FLIP_DISPLAY,
		CREATE_IMAGE,
		CREATE_TEXTURE,
		CREATE_FONT,
		GET_EVENT,

		_NUM_METHODS
	};

public:
	IVoodooGraphics(Voodoo::Client& client, Voodoo::ID method_id)
		:
		InterfaceClient(client, method_id)
	{
	}

	void FillRectangle(sf::Vector2f pos, sf::Vector2f size, sf::Color color)
	{
		client.Call(method_id, (int)FILL_RECTANGLE, pos.x, pos.y, size.x, size.y, color.r, color.g, color.b, color.a);
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

	void TextureTriangle(const Triangle& triangle, InterfaceClient* texture)
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

	void RenderVertexArray(const sf::VertexArray& array, InterfaceClient* texture = 0)
	{
		client.Call2(method_id, &array[0], array.getVertexCount() * sizeof(array[0]),
					 (int)RENDER_VERTEXARRAY, array.getVertexCount(), (int)array.getPrimitiveType(),
					 texture ? texture->GetMethodID() : Voodoo::ID());
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

	Voodoo::ID CreateTexture(InterfaceClient* image)
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
	using Method = enum {
		RELEASE,

		WRITE,
		LOAD,

		_NUM_METHODS
	};

public:
	IVoodooImage(Voodoo::Client& client, Voodoo::ID method_id)
		:
		InterfaceClient(client, method_id)
	{
	}

	void Write(sf::IntRect rect, const void* data, int pitch)
	{
		for (int y = 0; y < rect.height; y++)
			client.Call2(method_id, (const char*)data + pitch * y, rect.width * 4, (int)WRITE, rect.left, rect.top + y, rect.width);
	}

	void LoadFromFile(std::string filename)
	{
		FILE* f;

#ifdef _WIN32
		if (fopen_s(&f, filename.c_str(), "r"))
#else
		if ((f = fopen(filename.c_str(), "r")) == NULL)
#endif
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

class IVoodooTexture : public Voodoo::InterfaceClient
{
public:
	using Method = enum {
		RELEASE,

		_NUM_METHODS
	};

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
	using Method = enum {
		RELEASE,

		LOAD,

		_NUM_METHODS
	};

public:
	IVoodooFont(Voodoo::Client& client, Voodoo::ID method_id)
		:
		InterfaceClient(client, method_id)
	{
	}

	void LoadFromFile(std::string filename)
	{
		FILE* f;

#ifdef _WIN32
		if (fopen_s(&f, filename.c_str(), "r"))
#else
		if ((f = fopen(filename.c_str(), "r")) == NULL)
#endif
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



class IVoodooImage_Server : public Voodoo::InterfaceServer<IVoodooImage>
{
private:
	std::array<std::function<std::any(std::vector<std::any>)>, IVoodooImage::_NUM_METHODS> dispatch;
	sf::Image image;

public:
	IVoodooImage_Server(Voodoo::Server& server, int width, int height)
		:
		InterfaceServer(server)
	{
		image.create(width, height);

		dispatch[IVoodooImage::WRITE] = [this](std::vector<std::any> args) -> std::any
		{
			write_image(std::any_cast<int>(args[1]),
						std::any_cast<int>(args[2]),
						std::any_cast<int>(args[3]),
						std::any_cast<const void*>(args[4]));

			return 0;
		};

		dispatch[IVoodooImage::LOAD] = [this](std::vector<std::any> args) -> std::any
		{
			//std::cout << (const char*)std::any_cast<const void*>(args[2]) << std::endl;
			image.loadFromMemory(std::any_cast<const void*>(args[2]), std::any_cast<int>(args[1]));
			//image.loadFromFile("bitmap.png");

			return 0;
		};
	}

	virtual std::function<std::any(std::vector<std::any>)> Lookup(IVoodooImage::Method method) const
	{
		return dispatch[method];
	}

	sf::Image& GetImage()
	{
		return image;
	}

private:
	void write_image(int x, int y, int width, const void* data)
	{
		sf::Image src;

		src.create(width, 1, (const sf::Uint8*)data);

		image.copy(src, x, y);
	}
};

class IVoodooTexture_Server : public Voodoo::InterfaceServer<IVoodooTexture>
{
private:
	std::array<std::function<std::any(std::vector<std::any>)>, IVoodooTexture::_NUM_METHODS> dispatch;
	sf::Texture texture;

public:
	IVoodooTexture_Server(Voodoo::Server& server, IVoodooImage_Server* image)
		:
		InterfaceServer(server)
	{
		texture.loadFromImage(image->GetImage());
	}

	virtual std::function<std::any(std::vector<std::any>)> Lookup(IVoodooTexture::Method method) const
	{
		return dispatch[method];
	}

	sf::Texture& GetTexture()
	{
		return texture;
	}
};

class IVoodooFont_Server : public Voodoo::InterfaceServer<IVoodooFont>
{
private:
	std::array<std::function<std::any(std::vector<std::any>)>, IVoodooFont::_NUM_METHODS> dispatch;
	sf::Font font;

public:
	IVoodooFont_Server(Voodoo::Server& server)
		:
		InterfaceServer(server)
	{
		dispatch[IVoodooFont::LOAD] = [this](std::vector<std::any> args) -> std::any
		{
			//std::cout << (const char*)std::any_cast<const void*>(args[2]) << std::endl;
			//font.loadFromMemory(std::any_cast<const void*>(args[2]), std::any_cast<int>(args[1]));
			font.loadFromFile("FreeSans.ttf");

			return 0;
		};
	}

	virtual std::function<std::any(std::vector<std::any>)> Lookup(IVoodooFont::Method method) const
	{
		return dispatch[method];
	}

	sf::Font& GetFont()
	{
		return font;
	}
};


class IVoodooGraphics_Server : public Voodoo::InterfaceServer<IVoodooGraphics>
{
private:
	std::array<std::function<std::any(std::vector<std::any>)>, IVoodooGraphics::_NUM_METHODS> dispatch;
	sf::RenderWindow window;

public:
	IVoodooGraphics_Server(Voodoo::Server& server)
		:
		InterfaceServer(server),
		window(sf::VideoMode(1024, 768), "Voodoo Graphics")
	{
		dispatch[IVoodooGraphics::FILL_RECTANGLE] = [this](std::vector<std::any> args) -> std::any
		{
			fill_rectangle(args);

			return 0;
		};

		dispatch[IVoodooGraphics::DRAW_SPRITE] = [this](std::vector<std::any> args) -> std::any
		{
			draw_sprite(args);

			return 0;
		};

		dispatch[IVoodooGraphics::DRAW_SPRITE_SCALED] = [this](std::vector<std::any> args) -> std::any
		{
			draw_sprite_scaled(args);

			return 0;
		};

		dispatch[IVoodooGraphics::TEXTURE_TRIANGLE] = [this](std::vector<std::any> args) -> std::any
		{
			texture_triangle(args);

			return 0;
		};

		dispatch[IVoodooGraphics::DRAW_TEXT] = [this](std::vector<std::any> args) -> std::any
		{
			draw_text(args);

			return 0;
		};

		dispatch[IVoodooGraphics::RENDER_VERTEXARRAY] = [this](std::vector<std::any> args) -> std::any
		{
			render_vertexarray(args);

			return 0;
		};

		dispatch[IVoodooGraphics::FLIP_DISPLAY] = [this](std::vector<std::any> args) -> std::any
		{
			flip_display();

			return 0;
		};

		dispatch[IVoodooGraphics::CREATE_IMAGE] = [this,&server](std::vector<std::any> args) -> std::any
		{
			auto image = new IVoodooImage_Server(server, std::any_cast<int>(args[1]), std::any_cast<int>(args[2]));

			return image->GetMethodID();
		};

		dispatch[IVoodooGraphics::CREATE_TEXTURE] = [this,&server](std::vector<std::any> args) -> std::any
		{
			auto texture = new IVoodooTexture_Server(server, (IVoodooImage_Server*)server.LookupInterface(std::any_cast<Voodoo::ID>(args[1])));

			return texture->GetMethodID();
		};

		dispatch[IVoodooGraphics::CREATE_FONT] = [this,&server](std::vector<std::any> args) -> std::any
		{
			auto font = new IVoodooFont_Server(server);

			return font->GetMethodID();
		};

		dispatch[IVoodooGraphics::GET_EVENT] = [this](std::vector<std::any> args) -> std::any
		{
			return get_event();
		};
	}

	virtual std::function<std::any(std::vector<std::any>)> Lookup(IVoodooGraphics::Method method) const
	{
		return dispatch[method];
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

	void render_vertexarray(std::vector<std::any> args)
	{
		auto num = std::any_cast<sf::Uint64>(args[1]);
		auto type = std::any_cast<int>(args[2]);
		auto tex = std::any_cast<Voodoo::ID>(args[3]);
		const sf::Vertex* v = static_cast<const sf::Vertex*>(std::any_cast<const void*>(args[4]));

		sf::RenderStates states = sf::RenderStates::Default;

		if (tex) {
			IVoodooTexture_Server* texture = (IVoodooTexture_Server*)server.LookupInterface(tex);

			states.texture = &texture->GetTexture();
		}

		sf::VertexArray arr((sf::PrimitiveType)type, num);

		/* FIXME: can we use memcpy instead? */
		for (size_t i = 0; i < num; i++)
			arr[i] = v[i];

		window.draw(arr, states);
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
	parallel_f::system::instance().setDebugLevel("Voodoo::Host", 0);
	parallel_f::system::instance().setDebugLevel("Voodoo::Server", 0);

	parallel_f::system::instance().setAutoFlush(parallel_f::system::AutoFlush::EndOfLine);


	Voodoo::Server server;
	Voodoo::Client client;

	VoodooTest::Setup setup(server, client);


	Voodoo::ID graphics_id = 1;	// In this case we know the ID that is used on the server to register

	std::unique_ptr<std::thread> server_loop;

	if (setup.test_server) {
		graphics_id = server.Register([&server](std::vector<std::any> args)
			{
				auto graphics = new IVoodooGraphics_Server(server);

				return graphics->GetMethodID();
			});

		server_loop = std::make_unique<std::thread>([&server]()
			{
				server.Run();
			});
	}


	if (setup.test_client) {
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

		sf::Clock clock;
		int       frames = 0;
		char      fps[10] = "";

		while (!windowClosed) {
			if (clock.getElapsedTime().asSeconds() >= 2) {
				snprintf( fps, 10, "%4.1f FPS", frames / clock.restart().asSeconds() );
				frames = 0;
			}

			frames++;

			graphics->FillRectangle(sf::Vector2f(100, 100), sf::Vector2f(400, 300), sf::Color(255, 0, 0, 255));
			graphics->FillRectangle(sf::Vector2f(300, 150), sf::Vector2f(400, 300), sf::Color(0, 0, 255, 255));
			graphics->FillRectangle(sf::Vector2f(150, 300), sf::Vector2f(400, 300), sf::Color(100, 100, 100, 255));

			graphics->DrawSprite(sf::Vector2f((float)(110 + rand() % 100), (float)(110 + rand() % 100)), texture);
			graphics->DrawSprite(sf::Vector2f((float)(300 + rand() % 100), (float)(200 + rand() % 100)), texture);
			graphics->DrawSprite(sf::Vector2f((float)(140 + rand() % 100), (float)(350 + rand() % 100)), texture);

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

			graphics->DrawText(sf::Vector2f(850, 30), font, 30, fps, sf::Color(250, 50, 50, 255));


			sf::VertexArray va(sf::PrimitiveType::TrianglesFan, 7);

			va[0] = sf::Vertex(sf::Vector2f(400.0f, 300.0f), sf::Color(255, 255, 255));

			for (int i = 1; i < 7; i++)
				va[i] = sf::Vertex(va[0].position + sf::Vector2f(i*50.0f, 300.0f-i*40.0f), sf::Color(i*190, 200-i*30, i*40));

			graphics->RenderVertexArray(va);


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
	}
	else
		setup.wait_server();


	if (setup.test_server)
		server.Stop();

	if (server_loop)
		server_loop->join();

	return 0;
}
