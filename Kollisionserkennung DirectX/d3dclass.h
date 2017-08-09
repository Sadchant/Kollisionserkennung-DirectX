#ifndef _D3DCLASS_H_
#define _D3DCLASS_H_

// Bibliotheken werden gelinkt
// beinhalten Funktionalität, um Direct3D zu initialisieren und 3D-Grafik zu zeichnen
// außerdem Tools um Bildwiederholrate und Grafikkarteninfos auszulesen
// sind auch DirectX-10-libs dabei (weil sie die selbe Funktionalität bieten)
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
//#pragma comment(lib, "d3dx11.lib")
//#pragma comment(lib, "d3dx10.lib")

//die entsprechenden Header includieren
#include <dxgi.h>
#include <d3dcommon.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <iostream>

#include <vector>

#include "Util.h"

using namespace DirectX;

using namespace std;

class D3DClass
{
public:
	D3DClass();
	D3DClass(const D3DClass&);
	~D3DClass();

	bool Initialize(int, int, bool, HWND, bool, float, float);
	void Shutdown();

	void BeginScene(float, float, float, float);
	void EndScene();

	ID3D11Device* GetDevice();
	ID3D11DeviceContext* GetDeviceContext();

	void GetProjectionMatrix(XMMATRIX&);
	void GetWorldMatrix(XMMATRIX&);
	void GetOrthoMatrix(XMMATRIX&);

	void GetVideoCardInfo(char*, int&);

	void TurnZBufferOn();
	void TurnZBufferOff();

	void TurnOnAlphaBlending();
	void TurnOffAlphaBlending();

	void TurnOnWireframeFillMode();
	void TurnOffWireframeFillMode();

private:
	bool m_vsync_enabled;
	int m_videoCardMemory;
	char m_videoCardDescription[128];
	IDXGISwapChain* m_swapChain; // implements one or more surfaces for storing rendered data before presenting it to an output.
	ID3D11Device* m_device; // mit dem Ding kann man ne Menge managen und man kann verschiedene Shader erzeugen!
	ID3D11DeviceContext* m_deviceContext; // represents a device context which generates rendering commands
	ID3D11RenderTargetView* m_renderTargetView; // vermutlich der Backbuffer auf den gerendert wird, identifies the render-target subresources that can be accessed during rendering
	ID3D11Texture2D* m_depthStencilBuffer; // wird zwischendurch beim Initialisieren benötigt, aber zunächst nicht beim Rendern
	ID3D11DepthStencilState* m_depthStencilState; // holds a description for depth-stencil state that you can bind to the output-merger stage, wird auch nicht beim Rendern benötigt
	ID3D11DepthStencilView* m_depthStencilView; // accesses a texture resource during depth-stencil testing
	ID3D11RasterizerState* m_rasterState;

	D3D11_RASTERIZER_DESC m_RasterDesc; // Describes rasterizer state, Struktur mit Daten, wie der Rasterisierer so rasterisieren soll


	XMMATRIX m_projectionMatrix;
	XMMATRIX m_worldMatrix;
	XMMATRIX m_orthoMatrix;

	ID3D11DepthStencilState* m_depthDisabledStencilState;

	ID3D11BlendState* m_alphaEnableBlendingState;
	ID3D11BlendState* m_alphaDisableBlendingState;
};

#endif