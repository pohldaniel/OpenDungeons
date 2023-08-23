#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <cstring>
#include <windows.h>

#include <Ogre.h>
#include <OIS.h>

INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT) {
	AllocConsole();
	AttachConsole(GetCurrentProcessId());
	freopen("CON", "w", stdout);
	freopen("CON", "w", stderr);
	SetConsoleTitle("Debug console");

	MoveWindow(GetConsoleWindow(), 1300, 0, 550, 300, true);
	std::cout << "right mouse      : capture game" << std::endl;
	std::cout << "space            : capture ui" << std::endl;
	std::cout << "ESC              : quit" << std::endl;
	std::cout << "v                : toggle vsync" << std::endl;
	std::cout << "alt + enter      : fullscreen" << std::endl;


	std::string resourcePath;


	resourcePath = "";

	Ogre::Root* root = new Ogre::Root(resourcePath + "plugins.cfg", "", "ogre.log");
	//Ogre::Root::setRenderSystem()
	//if (!root->showConfigDialog())
		//return -1;
	
		// Use the first available renderer
		//Ogre::RenderSystem* renderSys = root->getAvailableRenderers()[0];
		//root->setRenderSystem(renderSys);

	Ogre::ConfigFile cf;
	cf.load(resourcePath + "resources.cfg");

	// Go through all sections & settings in the file
	Ogre::ConfigFile::SectionIterator seci = cf.getSectionIterator();

	Ogre::String secName, typeName, archName;
	while (seci.hasMoreElements())
	{
		secName = seci.peekNextKey();
		Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext();
		Ogre::ConfigFile::SettingsMultiMap::iterator i;
		for (i = settings->begin(); i != settings->end(); ++i)
		{
			typeName = i->first;
			archName = i->second;



			Ogre::ResourceGroupManager::getSingleton().addResourceLocation(archName, typeName, secName);
		}
	}
	std::cout << "wwwwwwwwwwwwwwwwwww" << std::endl;
	Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
	std::cout << "wwwwwwwwwwwwwwwwwww" << std::endl;
	Ogre::RenderWindow* window = root->initialise(true);
	Ogre::SceneManager* smgr = root->createSceneManager(Ogre::ST_GENERIC);
	
	//Input Stuff
	size_t windowHnd = 0;
	window->getCustomAttribute("WINDOW", &windowHnd);
	OIS::InputManager* im = OIS::InputManager::createInputSystem(windowHnd);
	OIS::Keyboard* keyboard = static_cast<OIS::Keyboard*>(im->createInputObject(OIS::OISKeyboard, true));
	
	while (1)
	{
		Ogre::WindowEventUtilities::messagePump();

		keyboard->capture();

		if (keyboard->isKeyDown(OIS::KC_ESCAPE))
			break;

		if (root->renderOneFrame() == false)
			break;
	}

	im->destroyInputObject(keyboard);
	im->destroyInputSystem(im);
	im = 0;

	delete root;
	while (true) {

	}
	return 0;

	/*Application application;
	try
	{
		application.startApplication();
	}
	catch (Ogre::Exception &e)
	{
		std::cerr << "An exception has occured: " << e.getFullDescription() << std::endl;
	}
	return 0;*/
}