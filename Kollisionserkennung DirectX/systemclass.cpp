#include "systemclass.h"

SystemClass::SystemClass()
{
	m_Input = NULL;
	m_Graphics = NULL;
}

SystemClass::SystemClass(const SystemClass& other)
{
}

SystemClass::~SystemClass()
{
}

bool SystemClass::Initialize()
{
	int screenWidth, screenHeight;
	bool result;


	// Initialize the width and height of the screen to zero before sending the variables into the function.
	screenWidth = 0;
	screenHeight = 0;

	// Initialize the windows api.
	InitializeWindows(screenWidth, screenHeight);

	// Create the input object.  This object will be used to handle reading the keyboard input from the user.
	m_Input = new InputClass;
	if (!m_Input)
	{
		return false;
	}

	// Initialize the input object.
	m_Input->Initialize();

	// Create the graphics object.  This object will handle rendering all the graphics for this application.
	m_Graphics = new GraphicsClass;
	if (!m_Graphics)
	{
		return false;
	}

	// Initialize the graphics object.
	result = m_Graphics->Initialize(screenWidth, screenHeight, m_hwnd);
	if (!result)
	{
		return false;
	}

	return true;
}

void SystemClass::Shutdown()
{
	// Release the graphics object.
	if (m_Graphics)
	{
		m_Graphics->Shutdown();
		delete m_Graphics;
		m_Graphics = 0;
	}

	// Release the input object.
	if (m_Input)
	{
		delete m_Input;
		m_Input = 0;
	}

	// Shutdown the window.
	ShutdownWindows();

	return;
}

void SystemClass::Run()
{
	MSG msg;
	bool done, result;


	// Initialize the message structure.
	ZeroMemory(&msg, sizeof(MSG));

	// Loop until there is a quit message from the window or the user.
	done = false;
	while (!done)
	{
		// Handle the windows messages.
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// If windows signals to end the application then exit out.
		if (msg.message == WM_QUIT)
		{
			done = true;
		}
		else
		{
			// Otherwise do the frame processing.
			result = Frame();
			if (!result)
			{
				done = true;
			}
		}

	}

	return;

}

bool SystemClass::Frame()
{
	bool result;


	// Check if the user pressed escape and wants to exit the application.
	if (m_Input->IsKeyDown(VK_ESCAPE))
	{
		return false;
	}

	// Do the frame processing for the graphics object.
	result = m_Graphics->Frame();
	if (!result)
	{
		return false;
	}

	return true;
}

LRESULT CALLBACK SystemClass::MessageHandler(HWND hwnd,			// wird an DefWindowProc weitergeleitet 
											UINT umsg,			// beinhaltet WM_KEYDOWN, WM_KEYUP etc.
											WPARAM wparam,		// beinhaltet die Taste, die gedrückt wurde
											LPARAM lparam)		// wird an DefWindowProc weitergeleitet 
{
	switch (umsg)
	{
		// Check if a key has been pressed on the keyboard.
	case WM_KEYDOWN:
	{
					   // If a key is pressed send it to the input object so it can record that state.
					   m_Input->KeyDown((unsigned int)wparam);
					   return 0;
	}

		// Check if a key has been released on the keyboard.
	case WM_KEYUP:
	{
					 // If a key is released then send it to the input object so it can unset the state for that key.
					 m_Input->KeyUp((unsigned int)wparam);
					 return 0;
	}

		// Any other messages send to the default message handler as our application won't make use of them.
	default:
	{
			   //stellt sicher, dass jede message verarbeitet wird
			   return DefWindowProc(hwnd, umsg, wparam, lparam);
	}
	}
}

void SystemClass::InitializeWindows(int& screenWidth, int& screenHeight)
{
	// eine Struktur für die Instanz eines Fensters (oder so)
	WNDCLASSEX wc;		

	// Eigenschaften von Monitor (und Drucker)
	DEVMODE dmScreenSettings;
	int posX, posY;


	// Get an external pointer to this object.
	// statischer Zeiger auf die systemclass
	ApplicationHandle = this;

	// Get the instance of this application.
	// wird gebraucht, um die Instanz des Fensters zu befüllen, als Parameter den Namen des schon geladenen Moduls übergeben (oder halt nix)
	m_hinstance = GetModuleHandle(NULL);

	// Give the application a name.
	// kommt auch in die Instanz des Fensters
	m_applicationName = L"Kollisionserkennung DirectX";

	// Setup the windows class with default settings.
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;			// wird es bei Bewegung neu gezeichnet? Hat es einen Schatten?
	wc.lpfnWndProc = WndProc;								// Zeiger auf Funktion die ein LRESULT zurückgibt und sich um Events kümmert
	wc.cbClsExtra = 0;										// Anzahl der Bytes, die hinter der Window-Struktor allokiert werden sollen
	wc.cbWndExtra = 0;										// Anzahl der Bytes, die hinter der Fenster-Instanz allokiert werden sollen (DLGWINDOWEXTRA, wenn dialogbox benutzt werden soll)
	wc.hInstance = m_hinstance;								// handle der Instanz, die die Fenster-Methode beinhaltet
	wc.hIcon												// handle des Icons der Klasse
				= LoadIcon(NULL, IDI_WINLOGO);					// erster Parameter NULL, da StandartIcon, zweiter ein Flag
	wc.hIconSm = wc.hIcon;									// handle eines kleinen Icons
	wc.hCursor												// handle des Klassen-Maus-Cursors
				= LoadCursor(NULL, IDC_ARROW);					// erster Parameter NULL, da keine Module-Instanz vorhanden, zweiter ein Flag
	wc.hbrBackground										// handle des Klassen-Hintergrunds
				= (HBRUSH)GetStockObject(BLACK_BRUSH);			// gibt handle auf stock fonts/pens/brushes zurück
	wc.lpszMenuName = NULL;									// Zeiger auf einen Resourcen-Namen des Klassen-Menus, bei NULL gibt es kein Default-Menu
	wc.lpszClassName = m_applicationName;					// atom oder String, bei String der Name des Fensters
	wc.cbSize = sizeof(WNDCLASSEX);							// Größe der Struktur

	// Register the window class.
	RegisterClassEx(&wc);									// erzeugt ein Fenster mit extended Fenster-Style

	// Determine the resolution of the clients desktop screen.
	// GetSystemMetrics kann Tonnen an SystemInfos herausfinden wie Anzahl angeschlossener Bildschirme (SM_CMONITORS), 
	// Anzahl der Maustasten (SM_CMOUSEBUTTONS), wie gebootet wurde (SM_CLEANBOOT)
	// und tonnenweise Formatierungsdaten für Fenster, wie die Höhe von Buttons mit kleinen Überschriften (SM_CYSMSIZE),
	// Größenangaben in Pixeln
	screenWidth = GetSystemMetrics(SM_CXSCREEN);
	screenHeight = GetSystemMetrics(SM_CYSCREEN);

	// Setup the screen settings depending on whether it is running in full screen or in windowed mode.
	if (FULL_SCREEN)
	{
		// If full screen set the screen to maximum size of the users desktop and 32bit.
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth = (unsigned long)screenWidth;
		dmScreenSettings.dmPelsHeight = (unsigned long)screenHeight;
		dmScreenSettings.dmBitsPerPel = 32;
		dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		// Change the display settings to full screen.
		ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);

		// Set the position of the window to the top left corner.
		posX = posY = 0;
	}
	else
	{
		// If windowed then set it to 800x600 resolution.
		screenWidth = 800;
		screenHeight = 600;

	// Place the window in the middle of the screen.
		posX = (GetSystemMetrics(SM_CXSCREEN) - screenWidth) / 2;
		posY = (GetSystemMetrics(SM_CYSCREEN) - screenHeight) / 2;
	}

	// Create the window with the screen settings and get the handle to it.
	m_hwnd = CreateWindowEx(WS_EX_APPWINDOW,
							m_applicationName, 
							m_applicationName,
							//WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP,
							WS_VISIBLE | WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU,
							posX, 
							posY, 
							screenWidth, 
							screenHeight, 
							NULL, 
							NULL, 
							m_hinstance, 
							NULL);

	// Bring the window up on the screen and set it as main focus.
	ShowWindow(m_hwnd, SW_SHOW);
	SetForegroundWindow(m_hwnd);
	SetFocus(m_hwnd);

	// Hide the mouse cursor.
	//ShowCursor(false);

	// screenwidth und height auf die richtigen werte setzen für den output der funktion, da das fenster eine Fensterleiste bekommen hat
	RECT windowRect;
	GetClientRect(m_hwnd, &windowRect);
	screenWidth = windowRect.right - windowRect.left;
	screenHeight = windowRect.bottom - windowRect.top;

	return;
}

void SystemClass::ShutdownWindows()
{
	// Show the mouse cursor.
	ShowCursor(true);

	// Fix the display settings if leaving full screen mode.
	if (FULL_SCREEN)
	{
		ChangeDisplaySettings(NULL, 0);
	}

	// Remove the window.
	DestroyWindow(m_hwnd);
	m_hwnd = NULL;

	// Remove the application instance.
	UnregisterClass(m_applicationName, m_hinstance);
	m_hinstance = NULL;

	// Release the pointer to this class.
	ApplicationHandle = NULL;

	return;
}

// LRESULT: Zeiger auf long-Variable
// CALLBACK: Zeiger auf eine Funktion
LRESULT CALLBACK WndProc(HWND hwnd,		// "Henkel" von einem Fenster, letztendlich ein Zeiger auf ein Fenster |||| WIRD NUR AN MH WEITERGEREICHT
						UINT umessage,	// unsigned integer, darüber wird gesitcht
						WPARAM wparam,	// message paramter (wtf?), letztendlich ein unsigned int der auch 64 bit groß sein kann |||| WIRD NUR AN MH WEITERGEREICHT
						LPARAM lparam)	// message parameter |||| WIRD NUR AN MH WEITERGEREICHT
{
	// gibt entweder 0 zurück wenn umessage WMDESTROY oder CLOSE ist, ansonsten das ergebnis von messagehandler()
	switch (umessage)
	{
		// Check if the window is being destroyed.
	case WM_DESTROY:
	{
					   PostQuitMessage(0); // sagt dem System, dass der Thread geschlossen werden will
					   return 0;
	}

		// Check if the window is being closed.
	case WM_CLOSE:
	{
					 PostQuitMessage(0);
					 return 0;
	}

		// All other messages pass to the message handler in the system class.
	default:
	{
			   return ApplicationHandle->MessageHandler(hwnd, umessage, wparam, lparam);
	}
	}
}