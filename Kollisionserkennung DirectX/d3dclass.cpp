#include "d3dclass.h"

D3DClass::D3DClass()
{
	m_swapChain = NULL;
	m_device = NULL;
	m_deviceContext = NULL;
	m_renderTargetView = NULL;
	m_depthStencilBuffer = NULL;
	m_depthStencilState = NULL;
	m_depthStencilView = NULL;
	m_rasterState = NULL;
	m_depthDisabledStencilState = 0;
	m_alphaEnableBlendingState = NULL;
	m_alphaDisableBlendingState = NULL;
}


D3DClass::D3DClass(const D3DClass& other)
{
}


D3DClass::~D3DClass()
{
}

// initialisiert Direct3D
// die Funktion befüllt mit Hilfe der am Anfang deklarierten lokalen Hilfsvariablen die Membervariablen der Klasse
// screenWidth/screenHeight, hwnd: Daten vom vorher erzeugten Fenster, damit Direct3D damit arbeiten kann
bool D3DClass::Initialize(int screenWidth, int screenHeight, bool vsync, HWND hwnd, bool fullscreen,
	float screenDepth, float screenNear)
{
	HRESULT result; // Wird von diversen Windows-Funktionen zurückgegeben, beschreibt Fehler/Warnung
	IDXGIFactory* factory; // Factory, die DXGI-Objekte erzeugen kann, die sich um Vollbild-Umschaltungen kümmern
	IDXGIAdapter* adapter; // repräsentiert ein "Display sub-system", also eigentlich eingebaute Grafikkarten
	//IDXGIOutput* adapterOutput; // An IDXGIOutput interface represents an adapter output (such as a monitor)
	unsigned int numerator, denominator;
	size_t stringLength; // kein uint, damit es für x64 compiliert
	//DXGI_MODE_DESC* displayModeList; // Describes a display mode, Struktur mit Höhe, Breite, Widerholrate, "Scaling"(wtf :P)
	DXGI_ADAPTER_DESC adapterDesc; // Describes an adapter (or video card), Struktur mit Infos über Grafikkarte mit IDs, Speichergröße usw.
	int error;
	DXGI_SWAP_CHAIN_DESC swapChainDesc; // Describes a swap chain, Struktur mit Backbuffer-Geraffel
	D3D_FEATURE_LEVEL featureLevel; // der bekannte Feature-Level von Direct3D, includiert je nachdem höhere Shader Model Versionen (???)
	ID3D11Texture2D* backBufferPtr; // A 2D texture interface manages texel data, which is structured memory, ist wohl dafür da um später drauf zu rendern
	D3D11_TEXTURE2D_DESC depthBufferDesc; // Describes a 2D texture, Struktur mit allen Infos über die Textur
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc; // Describes depth-stencil state, hat was mit Tiefeninformationen zu tun, vielleicht um nach dem Plattdrücken
											   // das richtige Objekt vorne zu rendern?
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc; // Specifies the subresources of a texture that are accessible from a depth-stencil view
	D3D11_VIEWPORT viewport; // Defines the dimensions of a viewport, A viewport is a way of translating pixel coordinates to normalized coordinates
							 // Pixelkoordinaten sind die normalen Fensterkoordinaten, normalisierte gehen von -1 bis 1
	float fieldOfView, screenAspect;

	D3D11_DEPTH_STENCIL_DESC depthDisabledStencilDesc;
	D3D11_BLEND_DESC blendStateDescription;


	// Store the vsync setting.
	m_vsync_enabled = vsync;


	// Create a DirectX graphics interface factory.
	// __uuidof: crazy Visual Studio operator, der die GUID (?) des Audrucks abruft
	// zweiter Parameter: da füllt die Funktion die Factory rein, muss aber ein void-Zeiger auf nen Zeiger sein
	result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);

	// FAILED: übersetzt das HRESULT in ein bool
	if (FAILED(result))
	{
		return false;
	}

	// Use the factory to create an adapter for the primary graphics interface (video card).
	// befüllt Paramter2 (ein Zeiger auf einen Zeiger auf IDXGIAdapter) mit Zeiger auf Zeiger auf ein Adapter-Interface (?)
	/*result = factory->EnumAdapters(2, &adapter);
	if (FAILED(result))
	{
		return false;
	}*/


	UINT j = 0;
	vector <IDXGIAdapter*> vAdapters;
	while (factory->EnumAdapters(j, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		vAdapters.push_back(adapter);
		++j;
	}

	// j - 2 sollte auf einem PC = 0 ergeben, auf einem Laptop = 1, somit dedizierte GPU
	// j - 1 ergibt den 
	if (j > 1)
		adapter = vAdapters[j - 2];
	else
		adapter = vAdapters[0];


	/*IDXGIOutput ** pOutput = new IDXGIOutput*[j];
	vector<IDXGIOutput*> vOutputs;
	for (j = 0; j < vAdapters.size(); j++)
	{
		UINT k = 0;
		while (vAdapters[j]->EnumOutputs(k, &*pOutput) != DXGI_ERROR_NOT_FOUND)
		{
			vOutputs.push_back(pOutput[k]);
			++k;
		}
	}
	IDXGIOutput *firstOutput = vOutputs[0];*/


	// Enumerate the primary adapter output (monitor).
	// befüllt adapterOutput mit dem Output-Ziel der Grafikkarte
	/*result = adapter->EnumOutputs(0, &adapterOutput);
	if (FAILED(result))
	{
		return false;
	}*/

	// Get the number of modes that fit the DXGI_FORMAT_R8G8B8A8_UNORM display format for the adapter output (monitor).
	// Starting with Direct3D 11.1, we recommend not to use GetDisplayModeList anymore to retrieve the matching display mode. 
	// Instead, use IDXGIOutput1::GetDisplayModeList1, which supports stereo display mode.
	// befüllt numModes mit der Anzahl der verfügbaren DisplayModi
	/*result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, NULL);
	if (FAILED(result))
	{
		return false;
	}*/

	// Create a list to hold all the possible display modes for this monitor/video card combination.
	/*displayModeList = new DXGI_MODE_DESC[numModes];
	if (!displayModeList)
	{
		return false;
	}*/

	// Now fill the display mode list structures.
	/*result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, displayModeList);
	if (FAILED(result))
	{
		return false;
	}*/

	// Now go through all the display modes and find the one that matches the screen width and height.
	// (die Display Modes haben nämlich alle möglichen Auflösungen
	// When a match is found store the numerator and denominator of the refresh rate for that monitor.
	/*for (i = 0; i<numModes; i++)
	{
		if (displayModeList[i].Width == (unsigned int)screenWidth)
		{
			if (displayModeList[i].Height == (unsigned int)screenHeight)
			{
				numerator = displayModeList[i].RefreshRate.Numerator;
				denominator = displayModeList[i].RefreshRate.Denominator;
			}
		}
	}*/
	// Setze die beiden auf den Wert der in der displaymodelist für 800x600 steht
	numerator = 56250;
	denominator = 1000;

	// Get the adapter (video card) description.
	// Gets a DXGI 1.0 description of an adapter(or video card).
	// in adapterDesc steht danach Name der Graka, Videospeicher etc
	result = adapter->GetDesc(&adapterDesc);
	if (FAILED(result))
	{
		return false;
	}

	// Store the dedicated video card memory in megabytes.
	m_videoCardMemory = (int)(adapterDesc.DedicatedVideoMemory / 1024 / 1024);

	// Convert the name of the video card to a character array and store it.
	error = wcstombs_s(&stringLength, m_videoCardDescription, 128, adapterDesc.Description, 128);
	if (error != 0)
	{
		return false;
	}

	// Release the display mode list.
	/*delete[] displayModeList;
	displayModeList = 0;*/

	// Release the adapter output.
	//adapterOutput->Release();
	//adapterOutput = 0;



	// Release the factory.
	factory->Release();
	factory = 0;

	// Initialize the swap chain description.
	// ZeroMemory füllt das was man reingetan hat mit 0en
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

	/*typedef struct DXGI_SWAP_CHAIN_DESC {
		DXGI_MODE_DESC   BufferDesc;
		DXGI_SAMPLE_DESC SampleDesc;
		DXGI_USAGE       BufferUsage;
		UINT             BufferCount;
		HWND             OutputWindow;
		BOOL             Windowed;
		DXGI_SWAP_EFFECT SwapEffect;
		UINT             Flags;
	} DXGI_SWAP_CHAIN_DESC;*/

	// der Bufferdesk:
	/*typedef struct DXGI_MODE_DESC {
	  UINT                     Width;
	  UINT                     Height;
	  DXGI_RATIONAL            RefreshRate;
	  DXGI_FORMAT              Format;
	  DXGI_MODE_SCANLINE_ORDER ScanlineOrdering;
	  DXGI_MODE_SCALING        Scaling;
	} DXGI_MODE_DESC;*/



	// Set to a single back buffer.
	swapChainDesc.BufferCount = 1;

	// Set the width and height of the back buffer.
	swapChainDesc.BufferDesc.Width = screenWidth;
	swapChainDesc.BufferDesc.Height = screenHeight;

	// Set regular 32-bit surface for the back buffer.
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// Set the refresh rate of the back buffer.
	if (m_vsync_enabled)
	{
		swapChainDesc.BufferDesc.RefreshRate.Numerator = numerator;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = denominator;
	}
	else
	{
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	}

	// Set the usage of the back buffer.
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	// Set the handle for the window to render to.
	swapChainDesc.OutputWindow = hwnd;

	// Turn multisampling off.
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;

	// Set to full screen or windowed mode.
	swapChainDesc.Windowed = !fullscreen;

	// Set the scan line ordering and scaling to unspecified.
	// Wie er das Bild aufbaut, ob oben unten progressiv...
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	// ob das Bild zentriert und nicht gescaled oder gestreckt wird (oder unpezifiziert wtf :O)
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Discard the back buffer contents after presenting.
	// DXGI_SWAP_EFFECT_DISCARD: Use this flag to enable the display driver to select the most efficient presentation technique for the swap chain.
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	// Don't set the advanced flags.
	swapChainDesc.Flags = 0;

	// Set the feature level to DirectX 11.
	featureLevel = D3D_FEATURE_LEVEL_11_1;

	// Create the swap chain, Direct3D device, and Direct3D device context.
	// Now that the swap chain description and feature level have been filled out we can create the swap chain, the Direct3D device, and the Direct3D device context. 
	// The Direct3D device and Direct3D device context are very important, they are the interface to all of the Direct3D functions. We will use the device and device
	// context for almost everything from this point forward.

	// D3D_DRIVER_TYPE_UNKNOWN benutzen, wenn man einen spezifischen adapter übergibt!
	result = D3D11CreateDeviceAndSwapChain(adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, D3D11_CREATE_DEVICE_DEBUG, &featureLevel, 1,
		D3D11_SDK_VERSION, &swapChainDesc, &m_swapChain, &m_device, NULL, &m_deviceContext);
	if (FAILED(result))
	{
		return false;
	}

	// Adapter-Vector wieder freigeben
	for (IDXGIAdapter* curAdapater : vAdapters)
	{
		curAdapater->Release();
	}
	vAdapters.clear();

	// checken ob alles gefunzt hat
	// Get the pointer to the back buffer.
	result = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBufferPtr);
	if (FAILED(result))
	{
		return false;
	}

	// Create the render target view with the back buffer pointer.
	result = m_device->CreateRenderTargetView(backBufferPtr, NULL, &m_renderTargetView);
	if (FAILED(result))
	{
		return false;
	}

	// Release pointer to the back buffer as we no longer need it.
	backBufferPtr->Release();
	backBufferPtr = 0;

	// We will also need to set up a depth buffer description.We'll use this to create a depth buffer so that our polygons can be rendered properly in 3D space.

	// Initialize the description of the depth buffer.
	// den depthBuffer mit 0en initialisieren
	ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));

	// Set up the description of the depth buffer.
	depthBufferDesc.Width = screenWidth;
	depthBufferDesc.Height = screenHeight;
	// Use 1 for a multisampled texture, hat was mit Antialiasing zu tun?!
	depthBufferDesc.MipLevels = 1;
	// man kann wohl ganz viele Texturen (bis zu 2048) reintun!?
	depthBufferDesc.ArraySize = 1;
	// Texturformat, gibt 132 (wtf), zB auch DXGI_FORMAT_420_OPAQUE
	depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	/*typedef struct DXGI_SAMPLE_DESC {
		UINT Count;
		UINT Quality;
	} DXGI_SAMPLE_DESC;*/
	// Count: The number of multisamples per pixel.
	// The default sampler mode, with no anti-aliasing, has a count of 1 and a quality level of 0.
	depthBufferDesc.SampleDesc.Count = 1;
	depthBufferDesc.SampleDesc.Quality = 0;

	// Value that identifies how the texture is to be read from and written to, zB D3D11_USAGE_IMMUTABLE
	depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	// Identifies how to bind a resource to the pipeline, zB auch D3D11_BIND_VERTEX_BUFFER, D3D11_BIND_SHADER_RESOURCE, D3D11_BIND_VIDEO_ENCODER, D3D11_BIND_RENDER_TARGET
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	// specify the types of CPU access allowed. Use 0 if CPU access is not required
	// D3D11_CPU_ACCESS_WRITE und D3D11_CPU_ACCESS_READ sind möglich
	depthBufferDesc.CPUAccessFlags = 0;
	// that identify other, less common resource options. Use 0 if none of these flags apply
	depthBufferDesc.MiscFlags = 0;



	// Now we create the depth/stencil buffer using that description. You will notice we use the CreateTexture2D function to make the buffers, hence the buffer is just a 2D texture. 
	// The reason for this is that once your polygons are sorted and then rasterized they just end up being colored pixels in this 2D buffer. Then this 2D buffer is drawn to the screen. 


	// Create the texture for the depth buffer using the filled out description.
	result = m_device->CreateTexture2D(&depthBufferDesc, NULL, &m_depthStencilBuffer);
	//									in				in opt	out
	if (FAILED(result))
	{
		return false;
	}


	// Now we need to setup the depth stencil description. This allows us to control what type of depth test Direct3D will do for each pixel.


	// Initialize the description of the stencil state.
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));

	// Set up the description of the stencil state.

	// Enable depth testing
	depthStencilDesc.DepthEnable = true;
	// D3D11_DEPTH_WRITE_MASK_ZERO: Turn off writes to the depth - stencil buffer.
	// D3D11_DEPTH_WRITE_MASK_ALL:	Turn on writes to the depth - stencil buffer.
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	// A function that compares depth data against existing depth data, gibt auch D3D11_COMPARISON_GREATER, D3D11_COMPARISON_ALWAYS usw.
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

	// Enable stencil testing
	depthStencilDesc.StencilEnable = true;
	// Identify a portion of the depth-stencil buffer for reading stencil data
	depthStencilDesc.StencilReadMask = 0xFF;
	// Identify a portion of the depth-stencil buffer for writing stencil data
	depthStencilDesc.StencilWriteMask = 0xFF;

	/*typedef struct D3D11_DEPTH_STENCILOP_DESC {
		D3D11_STENCIL_OP      StencilFailOp;
		D3D11_STENCIL_OP      StencilDepthFailOp;
		D3D11_STENCIL_OP      StencilPassOp;
		D3D11_COMPARISON_FUNC StencilFunc;
	} D3D11_DEPTH_STENCILOP_DESC;*/

	// Stencil operations if pixel is front-facing.
	// FrontFace: Identify how to use the results of the depth test and the stencil test for pixels whose surface normal is facing towards the camera

	// hier könnte als Wert auch D3D11_STENCIL_OP_REPLACE oder D3D11_STENCIL_OP_ZERO stehen
	// The stencil operation to perform when stencil testing fails
	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	// The stencil operation to perform when stencil testing passes and depth testing fails
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	// The stencil operation to perform when stencil testing and depth testing both pass
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	// A function that compares stencil data against existing stencil data, könnte auch D3D11_COMPARISON_NEVER hin
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;


	// BackFace: Identify how to use the results of the depth test and the stencil test for pixels whose surface normal is facing away from the camera
	// Stencil operations if pixel is back-facing.
	// die selben Variablen wie Front Face
	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Create the depth stencil state.
	result = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilState);
	if (FAILED(result))
	{
		return false;
	}

	// With the created depth stencil state we can now set it so that it takes effect. Notice we use the device context to set it. 
	// Set the depth stencil state.
	// Sets the depth-stencil state of the output-merger stage
	m_deviceContext->OMSetDepthStencilState(m_depthStencilState, 1);

	// Initailze the depth stencil view.
	ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));

	// Set up the depth stencil view description.
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;

	// Create the depth stencil view.
	result = m_device->CreateDepthStencilView(m_depthStencilBuffer, &depthStencilViewDesc, &m_depthStencilView);
	if (FAILED(result))
	{
		return false;
	}

	// With that created we can now call OMSetRenderTargets. This will bind the render target view and the depth stencil buffer to the output render pipeline. 
	// This way the graphics that the pipeline renders will get drawn to our back buffer that we previously created. With the graphics written to the back buffer 
	// we can then swap it to the front and display our graphics on the user's screen. 

	// Bind the render target view and depth stencil buffer to the output render pipeline.
	m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilView);



	// The rasterizer state will give us control over how polygons are rendered. We can do things like make our scenes render 
	// in wireframe mode or have DirectX draw both the front and back faces of polygons. By default DirectX already has a rasterizer state set up and working 
	// the exact same as the one below but you have no control to change it unless you set up one yourself. 


	// Setup the raster description which will determine how and what polygons will be drawn.

	/*typedef struct D3D11_RASTERIZER_DESC {
		D3D11_FILL_MODE FillMode;
		D3D11_CULL_MODE CullMode;
		BOOL            FrontCounterClockwise;
		INT             DepthBias;
		FLOAT           DepthBiasClamp;
		FLOAT           SlopeScaledDepthBias;
		BOOL            DepthClipEnable;
		BOOL            ScissorEnable;
		BOOL            MultisampleEnable;
		BOOL            AntialiasedLineEnable;
	} D3D11_RASTERIZER_DESC;*/

	// Specifies whether to enable line antialiasing; only applies if doing line drawing and MultisampleEnable is FALSE
	m_RasterDesc.AntialiasedLineEnable = false;

	// Indicates triangles facing the specified direction are not drawn
	// die angegebene Seite wird wohl nicht gerendert, es gibt NONE, FRONT und BACK
	m_RasterDesc.CullMode = D3D11_CULL_BACK;

	// Depth value added to a given pixel
	// wenn zwei Polygone an der selben Position liegen (zB bei Schatten) entscheided der höhere Depth Bias Wert, welches gerendert wird
	m_RasterDesc.DepthBias = 0;

	// Maximum depth bias of a pixel, wird hier wohl vorerst sowieso nicht benutzt
	m_RasterDesc.DepthBiasClamp = 0.0f;

	// Enable clipping based on distance.
	// The hardware always performs x and y clipping of rasterized coordinates.When DepthClipEnable is set to the default–TRUE, the hardware also clips the z value
	// (that is, the hardware performs the last step of the following algorithm).
	m_RasterDesc.DepthClipEnable = true;

	// Determines the fill mode to use when rendering triangles, D3D11_FILL_WIREFRAME geht auch
	m_RasterDesc.FillMode = /*D3D11_FILL_SOLID*/ D3D11_FILL_WIREFRAME;

	// Determines if a triangle is front- or back-facing. If this parameter is TRUE, a triangle will be considered front-facing if its vertices 
	// are counter-clockwise on the render target and considered back-facing if they are clockwise. If this parameter is FALSE, the opposite is true.
	m_RasterDesc.FrontCounterClockwise = false;

	// Specifies whether to use the quadrilateral or alpha line anti-aliasing algorithm on multisample antialiasing (MSAA) render targets
	m_RasterDesc.MultisampleEnable = false;

	// Enable scissor-rectangle culling. All pixels outside an active scissor rectangle are culled.
	m_RasterDesc.ScissorEnable = false;

	// Scalar on a given pixel's slope, hat noch was mit dem Depth Bias zu tun
	m_RasterDesc.SlopeScaledDepthBias = 0.0f;

	// Create the rasterizer state from the description we just filled out.
	result = m_device->CreateRasterizerState(&m_RasterDesc, &m_rasterState);
	if (FAILED(result))
	{
		return false;
	}

	// Now set the rasterizer state.
	m_deviceContext->RSSetState(m_rasterState);

	// The viewport also needs to be setup so that Direct3D can map clip space coordinates to the render target space. Set this to be the entire size of the window. 

	// Setup the viewport for rendering.
	viewport.Width = (float)screenWidth;
	viewport.Height = (float)screenHeight;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;

	// Create the viewport.
	m_deviceContext->RSSetViewports(1, &viewport);


	// Now we will create the projection matrix. The projection matrix is used to translate the 3D scene into the 2D viewport space that we previously created. 
	// We will need to keep a copy of this matrix so that we can pass it to our shaders that will be used to render our scenes. 
	// Setup the projection matrix.
	fieldOfView = (float)XM_PI / 4.0f;
	screenAspect = (float)screenWidth / (float)screenHeight;

	// Create the projection matrix for 3D rendering.
	// Builds a left-handed perspective projection matrix based on a field of view
	m_projectionMatrix = XMMatrixPerspectiveFovLH(fieldOfView, screenAspect, screenNear, screenDepth);

	// We will also create another matrix called the world matrix. This matrix is used to convert the vertices of our objects into vertices in the 3D scene. 
	// This matrix will also be used to rotate, translate, and scale our objects in 3D space. From the start we will just initialize the matrix to the identity 
	// matrix and keep a copy of it in this object. The copy will be needed to be passed to the shaders for rendering also. 
	// Initialize the world matrix to the identity matrix.
	// identity Matrix höchstwahrscheinlich die EinheitsMatrix
	m_worldMatrix = XMMatrixIdentity();

	// And the final thing we will setup in the Initialize function is an orthographic projection matrix. This matrix is used for rendering 2D elements like user 
	// interfaces on the screen allowing us to skip the 3D rendering

	// Create an orthographic projection matrix for 2D rendering.
	// Builds a left-handed orthographic projection matrix und füllt sie in Parameter 1
	m_orthoMatrix = XMMatrixOrthographicLH((float)screenWidth, (float)screenHeight, screenNear, screenDepth);

	// Clear the second depth stencil state before setting the parameters.
	ZeroMemory(&depthDisabledStencilDesc, sizeof(depthDisabledStencilDesc));

	// Now create a second depth stencil state which turns off the Z buffer for 2D rendering.  The only difference is 
	// that DepthEnable is set to false, all other parameters are the same as the other depth stencil state.
	depthDisabledStencilDesc.DepthEnable = false;
	depthDisabledStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthDisabledStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthDisabledStencilDesc.StencilEnable = true;
	depthDisabledStencilDesc.StencilReadMask = 0xFF;
	depthDisabledStencilDesc.StencilWriteMask = 0xFF;
	depthDisabledStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthDisabledStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthDisabledStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthDisabledStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	depthDisabledStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthDisabledStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthDisabledStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthDisabledStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	// Now create the new depth stencil.
		// Create the state using the device.
		result = m_device->CreateDepthStencilState(&depthDisabledStencilDesc, &m_depthDisabledStencilState);
	if (FAILED(result))
	{
		return false;
	}

	// First initialize the blend state description.
		// Clear the blend state description.
	ZeroMemory(&blendStateDescription, sizeof(D3D11_BLEND_DESC));

	// To create an alpha enabled blend state description change BlendEnable to TRUE and DestBlend to D3D11_BLEND_INV_SRC_ALPHA.
	// The other settings are set to their default values which can be looked up in the Windows DirectX Graphics Documentation.
		// Create an alpha enabled blend state description.
	blendStateDescription.RenderTarget[0].BlendEnable = TRUE;
	blendStateDescription.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blendStateDescription.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendStateDescription.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendStateDescription.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendStateDescription.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendStateDescription.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendStateDescription.RenderTarget[0].RenderTargetWriteMask = 0x0f;

	// We then create an alpha enabled blending state using the description we just setup.
		// Create the blend state using the description.
	result = m_device->CreateBlendState(&blendStateDescription, &m_alphaEnableBlendingState);
	if (FAILED(result))
	{
		return false;
	}
	// Now to create an alpha disabled state we change the description we just made to set BlendEnable to FALSE.The rest of the settings
	// can be left as they are.
		// Modify the description to create an alpha disabled blend state description.
	blendStateDescription.RenderTarget[0].BlendEnable = FALSE;
	// We then create an alpha disabled blending state using the modified blend state description.We now have two blending states we can
		// switch between to turn on and off alpha blending.
		// Create the blend state using the description.
	result = m_device->CreateBlendState(&blendStateDescription, &m_alphaDisableBlendingState);
	if (FAILED(result))
	{
		return false;
	}

	return true;
}

void D3DClass::Shutdown()
{
	// Before shutting down set to windowed mode or when you release the swap chain it will throw an exception.
	if (m_swapChain)
	{
		m_swapChain->SetFullscreenState(false, NULL);
	}
	if (m_alphaEnableBlendingState)
	{
		m_alphaEnableBlendingState->Release();
		m_alphaEnableBlendingState = 0;
	}

	if (m_alphaDisableBlendingState)
	{
		m_alphaDisableBlendingState->Release();
		m_alphaDisableBlendingState = 0;
	}
	if (m_depthDisabledStencilState)
	{
		m_depthDisabledStencilState->Release();
		m_depthDisabledStencilState = 0;
	}

	if (m_rasterState)
	{
		m_rasterState->Release();
		m_rasterState = 0;
	}

	if (m_depthStencilView)
	{
		m_depthStencilView->Release();
		m_depthStencilView = 0;
	}

	if (m_depthStencilState)
	{
		m_depthStencilState->Release();
		m_depthStencilState = 0;
	}

	if (m_depthStencilBuffer)
	{
		m_depthStencilBuffer->Release();
		m_depthStencilBuffer = 0;
	}

	if (m_renderTargetView)
	{
		m_renderTargetView->Release();
		m_renderTargetView = 0;
	}

	if (m_deviceContext)
	{
		m_deviceContext->Release();
		m_deviceContext = 0;
	}

	if (m_device)
	{
		m_device->Release();
		m_device = 0;
	}

	if (m_swapChain)
	{
		m_swapChain->Release();
		m_swapChain = 0;
	}

	return;
}

// The first new function TurnOnAlphaBlending allows us to turn on alpha blending by using the OMSetBlendState function with our 
// m_alphaEnableBlendingState blending state.
void D3DClass::TurnOnAlphaBlending()
{
	float blendFactor[4];


	// Setup the blend factor.
	blendFactor[0] = 0.0f;
	blendFactor[1] = 0.0f;
	blendFactor[2] = 0.0f;
	blendFactor[3] = 0.0f;

	// Turn on the alpha blending.
	m_deviceContext->OMSetBlendState(m_alphaEnableBlendingState, blendFactor, 0xffffffff);

	

	return;
}

// The second new function TurnOffAlphaBlending allows us to turn off alpha blending by using the OMSetBlendState function with our 
// m_alphaDisableBlendingState blending state.
void D3DClass::TurnOffAlphaBlending()
{
	float blendFactor[4];


	// Setup the blend factor.
	blendFactor[0] = 0.0f;
	blendFactor[1] = 0.0f;
	blendFactor[2] = 0.0f;
	blendFactor[3] = 0.0f;

	// Turn off the alpha blending.
	m_deviceContext->OMSetBlendState(m_alphaDisableBlendingState, blendFactor, 0xffffffff);

	return;
}

// In the D3DClass I have a couple helper functions. The first two are BeginScene and EndScene. BeginScene will be called whenever we are going to draw a new 3D scene 
// at the beginning of each frame. All it does is initializes the buffers so they are blank and ready to be drawn to
void D3DClass::BeginScene(float red, float green, float blue, float alpha)
{
	float color[4];


	// Setup the color to clear the buffer to.
	color[0] = red;
	color[1] = green;
	color[2] = blue;
	color[3] = alpha;

	// Clear the back buffer.
	m_deviceContext->ClearRenderTargetView(m_renderTargetView, color);

	// Clear the depth buffer.
	m_deviceContext->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	return;
}

//  it tells the swap chain to display our 3D scene once all the drawing has completed at the end of each frame. 
void D3DClass::EndScene()
{
	// Present the back buffer to the screen since rendering is complete.
	if (m_vsync_enabled)
	{
		// Lock to screen refresh rate.
		m_swapChain->Present(1, 0);
	}
	else
	{
		// Present as fast as possible.
		m_swapChain->Present(0, 0);
	}

	return;
}

ID3D11Device* D3DClass::GetDevice()
{
	return m_device;
}


ID3D11DeviceContext* D3DClass::GetDeviceContext()
{
	return m_deviceContext;
}

void D3DClass::GetProjectionMatrix(XMMATRIX& projectionMatrix)
{
	projectionMatrix = m_projectionMatrix;
	return;
}


void D3DClass::GetWorldMatrix(XMMATRIX& worldMatrix)
{
	worldMatrix = m_worldMatrix;
	return;
}


void D3DClass::GetOrthoMatrix(XMMATRIX& orthoMatrix)
{
	orthoMatrix = m_orthoMatrix;
	return;
}

void D3DClass::GetVideoCardInfo(char* cardName, int& memory)
{
	strcpy_s(cardName, 128, m_videoCardDescription);
	memory = m_videoCardMemory;
	return;
}

void D3DClass::TurnZBufferOn()
{
	m_deviceContext->OMSetDepthStencilState(m_depthStencilState, 1);
	return;
}


void D3DClass::TurnZBufferOff()
{
	m_deviceContext->OMSetDepthStencilState(m_depthDisabledStencilState, 1);
	return;
}

void D3DClass::TurnOnWireframeFillMode() 
{
	m_RasterDesc.FillMode = D3D11_FILL_WIREFRAME;
	HRESULT result = m_device->CreateRasterizerState(&m_RasterDesc, &m_rasterState);
	if (FAILED(result))
	{
		cout << "RABÄÄH RasterizerState konnte nicht created werden! :(" << endl;
	}
	m_deviceContext->RSSetState(m_rasterState);
}

void D3DClass::TurnOffWireframeFillMode()
{
	m_RasterDesc.FillMode = D3D11_FILL_SOLID;
	HRESULT result = m_device->CreateRasterizerState(&m_RasterDesc, &m_rasterState);
	if (FAILED(result))
	{
		cout << "RABÄÄH RasterizerState konnte nicht created werden! :(" << endl;
	}
	m_deviceContext->RSSetState(m_rasterState);
}



