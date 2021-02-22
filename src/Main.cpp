#include "Pomme.h"
#include "PommeInit.h"
#include "PommeFiles.h"
#include "PommeGraphics.h"

#include <SDL.h>

#include <iostream>

#if __APPLE__
#include <libproc.h>
#include <unistd.h>
#endif

extern "C"
{
	// Satisfy externs in game code
	SDL_Window*			gSDLWindow		= nullptr;
	SDL_Renderer*		gSDLRenderer	= nullptr;
	SDL_Texture*		gSDLTexture		= nullptr;
//	WindowPtr gCoverWindow = nullptr;
//	UInt32* gCoverWindowPixPtr = nullptr;

	// Lets the game know where to find its asset files
	FSSpec gDataSpec;

	void GameMain(void);
}

static fs::path FindGameData()
{
	fs::path dataPath;

#if __APPLE__
	char pathbuf[PROC_PIDPATHINFO_MAXSIZE];

	pid_t pid = getpid();
	int ret = proc_pidpath(pid, pathbuf, sizeof(pathbuf));
	if (ret <= 0)
	{
		throw std::runtime_error(std::string(__func__) + ": proc_pidpath failed: " + std::string(strerror(errno)));
	}

	dataPath = pathbuf;
	dataPath = dataPath.parent_path().parent_path() / "Resources";
#else
	dataPath = "Data";
#endif

	dataPath = dataPath.lexically_normal();

	// Set data spec
	gDataSpec = Pomme::Files::HostPathToFSSpec(dataPath / "Shapes");

	// Use application resource file
	auto applicationSpec = Pomme::Files::HostPathToFSSpec(dataPath / "System" / "Application");
	short resFileRefNum = FSpOpenResFile(&applicationSpec, fsRdPerm);
	UseResFile(resFileRefNum);

	return dataPath;
}

int CommonMain(int argc, const char** argv)
{
	// Start our "machine"
	Pomme::Init();

	// Uncomment to dump the game's resources to a temporary directory.
	Pomme_StartDumpingResources("/tmp/MikeRezDump");

	// Initialize SDL video subsystem
	if (0 != SDL_Init(SDL_INIT_VIDEO))
		throw std::runtime_error("Couldn't initialize SDL video subsystem.");

	// Create window
	gSDLWindow = SDL_CreateWindow(
			"Mighty Mike",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			640,
			480,
			SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);
	if (!gSDLWindow)
		throw std::runtime_error("Couldn't create SDL window.");

	gSDLRenderer = SDL_CreateRenderer(gSDLWindow, -1, SDL_RENDERER_PRESENTVSYNC);
	if (!gSDLRenderer)
		throw std::runtime_error("Couldn't create SDL renderer.");

	gSDLTexture = SDL_CreateTexture(gSDLRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 640, 480);
	if (!gSDLTexture)
		throw std::runtime_error("Couldn't create SDL texture.");

	SDL_RenderSetLogicalSize(gSDLRenderer, 640, 480);
	SDL_RenderSetIntegerScale(gSDLRenderer, SDL_TRUE);

	// Set up globals that the game expects
//	gCoverWindow = Pomme::Graphics::GetScreenPort();
//	gCoverWindowPixPtr = (UInt32*) GetPixBaseAddr(GetGWorldPixMap(gCoverWindow));

	fs::path dataPath = FindGameData();
#if !(__APPLE__)
	Pomme::Graphics::SetWindowIconFromIcl8Resource(gSDLWindow, 400);
#endif

	// Init joystick subsystem
	{
		SDL_Init(SDL_INIT_JOYSTICK);
		auto gamecontrollerdbPath8 = (dataPath / "System" / "gamecontrollerdb.txt").u8string();
		if (-1 == SDL_GameControllerAddMappingsFromFile((const char*)gamecontrollerdbPath8.c_str()))
		{
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Mighty Mike", "Couldn't load gamecontrollerdb.txt!", gSDLWindow);
		}
	}

	// Start the game
	try
	{
		GameMain();
	}
	catch (Pomme::QuitRequest&)
	{
		// no-op, the game may throw this exception to shut us down cleanly
	}

	// Clean up
	Pomme::Shutdown();

	return 0;
}

int main(int argc, char** argv)
{
	std::string uncaught;

	try
	{
		return CommonMain(argc, const_cast<const char**>(argv));
	}
	catch (std::exception& ex)
	{
		uncaught = ex.what();
	}
	catch (...)
	{
		uncaught = "unknown";
	}

	std::cerr << "Uncaught exception: " << uncaught << "\n";
	SDL_ShowSimpleMessageBox(0, "Uncaught Exception", uncaught.c_str(), nullptr);
	return 1;
}