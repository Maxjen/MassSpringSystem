#include <sstream>
#include <iomanip>
#include <random>
#include <iostream>
#include <cmath>

//DirectX includes
#include <DirectXMath.h>
using namespace DirectX;

// Effect framework includes
#include <d3dx11effect.h>

// DXUT includes
#include <DXUT.h>
#include <DXUTcamera.h>

// DirectXTK includes
#include "Effects.h"
#include "VertexTypes.h"
#include "PrimitiveBatch.h"
#include "GeometricPrimitive.h"
#include "ScreenGrab.h"

// AntTweakBar includes
#include "AntTweakBar.h"

// Internal includes
#include "util/util.h"

#include <vector>
using namespace std;

#include "Point.h"
#include "Spring.h"

bool isSimulating = false;

bool isStatic = false;
float mass = 1.0f;

float stiffness = 46.0f;
float initialLength = 0.5f;

bool addSpringMode = false;

float timeStep = 1.0f/60;

int selectedPoint = -1;
float selectedPointDepth;

vector<Point> points;
vector<Spring> springs;

struct Line
{
	float x1, y1, z1;
	float x2, y2, z2;
};

//vector<Line> lines;

int screenWidth;
int screenHeight;
float tanFOV;

// DXUT camera
// NOTE: CModelViewerCamera does not only manage the standard view transformation/camera position 
//       (CModelViewerCamera::GetViewMatrix()), but also allows for model rotation
//       (CModelViewerCamera::GetWorldMatrix()). 
//       Look out for CModelViewerCamera::SetButtonMasks(...).
CModelViewerCamera g_camera;

// Effect corresponding to "effect.fx"
ID3DX11Effect* g_pEffect = nullptr;

// Main tweak bar
TwBar* g_pTweakBar;

// DirectXTK effect, input layout and primitive batch for position/color vertices
// (for drawing multicolored & unlit primitives)
BasicEffect*                          g_pEffectPositionColor          = nullptr;
ID3D11InputLayout*                    g_pInputLayoutPositionColor     = nullptr;
PrimitiveBatch<VertexPositionColor>*  g_pPrimitiveBatchPositionColor  = nullptr;

// DirectXTK effect, input layout and primitive batch for position/normal vertices
// (for drawing unicolor & oriented & lit primitives)
BasicEffect*                          g_pEffectPositionNormal         = nullptr;
ID3D11InputLayout*                    g_pInputLayoutPositionNormal    = nullptr;
PrimitiveBatch<VertexPositionNormal>* g_pPrimitiveBatchPositionNormal = nullptr;

// DirectXTK simple geometric primitives
std::unique_ptr<GeometricPrimitive> g_pSphere;
std::unique_ptr<GeometricPrimitive> g_pTeapot;

// Movable object management
XMINT2   g_viMouseDelta = XMINT2(0,0);
XMFLOAT3 g_vfMovableObjectPos = XMFLOAT3(0,0,0);

// TweakAntBar GUI variables
int   g_iNumSpheres    = 100;
float g_fSphereSize    = 0.05f;
//bool  g_bDrawTeapot    = true;
bool  g_bDrawTeapot    = false;
bool  g_bDrawTriangle  = false;
//bool  g_bDrawSpheres   = true;
bool  g_bDrawSpheres   = false;

// Create TweakBar and add required buttons and variables
void InitTweakBar(ID3D11Device* pd3dDevice)
{
    TwInit(TW_DIRECT3D11, pd3dDevice);

    g_pTweakBar = TwNewBar("TweakBar");

    // HINT: For buttons you can directly pass the callback function as a lambda expression.
    TwAddButton(g_pTweakBar, "Reset Camera", [](void *){g_camera.Reset();}, nullptr, "");

	TwAddVarRW(g_pTweakBar, "Simulate World",   TW_TYPE_BOOLCPP, &isSimulating, "");
	TwAddVarRW(g_pTweakBar, "Static",	  TW_TYPE_BOOLCPP, &isStatic, "");
	TwAddVarRW(g_pTweakBar, "Mass",	  TW_TYPE_FLOAT, &mass, "min=0.01");
	TwAddVarRW(g_pTweakBar, "Stifness",	  TW_TYPE_FLOAT, &stiffness, "");
	TwAddVarRW(g_pTweakBar, "Initial Length",	  TW_TYPE_FLOAT, &initialLength, "");
    /*TwAddVarRW(g_pTweakBar, "Draw Teapot",   TW_TYPE_BOOLCPP, &g_bDrawTeapot, "");
    TwAddVarRW(g_pTweakBar, "Draw Triangle", TW_TYPE_BOOLCPP, &g_bDrawTriangle, "");
    TwAddVarRW(g_pTweakBar, "Draw Spheres",  TW_TYPE_BOOLCPP, &g_bDrawSpheres, "");
    TwAddVarRW(g_pTweakBar, "Num Spheres",   TW_TYPE_INT32, &g_iNumSpheres, "min=1");*/
    TwAddVarRW(g_pTweakBar, "Sphere Size",   TW_TYPE_FLOAT, &g_fSphereSize, "min=0.01 step=0.01");
}

// Draw the edges of the bounding box [-0.5;0.5]� rotated with the cameras model tranformation.
// (Drawn as line primitives using a DirectXTK primitive batch)
void DrawBoundingBox(ID3D11DeviceContext* pd3dImmediateContext)
{
    // Setup position/color effect
    g_pEffectPositionColor->SetWorld(g_camera.GetWorldMatrix());
    
    g_pEffectPositionColor->Apply(pd3dImmediateContext);
    pd3dImmediateContext->IASetInputLayout(g_pInputLayoutPositionColor);

    // Draw
    g_pPrimitiveBatchPositionColor->Begin();
    
    // Lines in x direction (red color)
    for (int i=0; i<4; i++)
    {
        g_pPrimitiveBatchPositionColor->DrawLine(
            VertexPositionColor(XMVectorSet(-0.5f, (float)(i%2)-0.5f, (float)(i/2)-0.5f, 1), Colors::Red),
            VertexPositionColor(XMVectorSet( 0.5f, (float)(i%2)-0.5f, (float)(i/2)-0.5f, 1), Colors::Red)
        );
    }

    // Lines in y direction
    for (int i=0; i<4; i++)
    {
        g_pPrimitiveBatchPositionColor->DrawLine(
            VertexPositionColor(XMVectorSet((float)(i%2)-0.5f, -0.5f, (float)(i/2)-0.5f, 1), Colors::Green),
            VertexPositionColor(XMVectorSet((float)(i%2)-0.5f,  0.5f, (float)(i/2)-0.5f, 1), Colors::Green)
        );
    }

    // Lines in z direction
    for (int i=0; i<4; i++)
    {
        g_pPrimitiveBatchPositionColor->DrawLine(
            VertexPositionColor(XMVectorSet((float)(i%2)-0.5f, (float)(i/2)-0.5f, -0.5f, 1), Colors::Blue),
            VertexPositionColor(XMVectorSet((float)(i%2)-0.5f, (float)(i/2)-0.5f,  0.5f, 1), Colors::Blue)
        );
    }

    g_pPrimitiveBatchPositionColor->End();
}

// Draw a large, square plane at y=-1.
// (Drawn as a quad, i.e. triangle strip, using a DirectXTK primitive batch)
void DrawFloor(ID3D11DeviceContext* pd3dImmediateContext)
{
    // Setup position/normal effect
    g_pEffectPositionNormal->SetWorld(XMMatrixIdentity());
    g_pEffectPositionNormal->SetEmissiveColor(Colors::Black);
    g_pEffectPositionNormal->SetDiffuseColor(0.6f * Colors::White);
    g_pEffectPositionNormal->SetSpecularColor(0.4f * Colors::White);
    g_pEffectPositionNormal->SetSpecularPower(1000);
    
    g_pEffectPositionNormal->Apply(pd3dImmediateContext);
    pd3dImmediateContext->IASetInputLayout(g_pInputLayoutPositionNormal);

    // Draw
    g_pPrimitiveBatchPositionNormal->Begin();
    g_pPrimitiveBatchPositionNormal->DrawQuad(
        VertexPositionNormal(XMFLOAT3(-100, -1,  100), XMFLOAT3(0,1,0)),
        VertexPositionNormal(XMFLOAT3( 100, -1,  100), XMFLOAT3(0,1,0)),
        VertexPositionNormal(XMFLOAT3( 100, -1, -100), XMFLOAT3(0,1,0)),
        VertexPositionNormal(XMFLOAT3(-100, -1, -100), XMFLOAT3(0,1,0))
    );
    g_pPrimitiveBatchPositionNormal->End();    
}

// Draw several objects randomly positioned in [-0.5f;0.5]�  using DirectXTK geometric primitives.
void DrawSomeRandomObjects(ID3D11DeviceContext* pd3dImmediateContext)
{
    // Setup position/normal effect (constant variables)
    g_pEffectPositionNormal->SetEmissiveColor(Colors::Black);
    g_pEffectPositionNormal->SetSpecularColor(0.4f * Colors::White);
    g_pEffectPositionNormal->SetSpecularPower(100);
      
    std::mt19937 eng;
    std::uniform_real_distribution<float> randCol( 0.0f, 1.0f);
    std::uniform_real_distribution<float> randPos(-0.5f, 0.5f);

    for (int i=0; i<g_iNumSpheres; i++)
    {
        // Setup position/normal effect (per object variables)
        g_pEffectPositionNormal->SetDiffuseColor(0.6f * XMColorHSVToRGB(XMVectorSet(randCol(eng), 1, 1, 0)));
        XMMATRIX scale    = XMMatrixScaling(g_fSphereSize, g_fSphereSize, g_fSphereSize);
        XMMATRIX trans    = XMMatrixTranslation(randPos(eng),randPos(eng),randPos(eng));
        g_pEffectPositionNormal->SetWorld(scale * trans * g_camera.GetWorldMatrix());

        // Draw
        // NOTE: The following generates one draw call per object, so performance will be bad for n>>1000 or so
        g_pSphere->Draw(g_pEffectPositionNormal, g_pInputLayoutPositionNormal);
    }
}

// Draw a teapot at the position g_vfMovableObjectPos.
void DrawMovableTeapot(ID3D11DeviceContext* pd3dImmediateContext)
{
    // Setup position/normal effect (constant variables)
    g_pEffectPositionNormal->SetEmissiveColor(Colors::Black);
    g_pEffectPositionNormal->SetDiffuseColor(0.6f * Colors::Cornsilk);
    g_pEffectPositionNormal->SetSpecularColor(0.4f * Colors::White);
    g_pEffectPositionNormal->SetSpecularPower(100);

    XMMATRIX scale    = XMMatrixScaling(0.5f, 0.5f, 0.5f);    
    XMMATRIX trans    = XMMatrixTranslation(g_vfMovableObjectPos.x, g_vfMovableObjectPos.y, g_vfMovableObjectPos.z);
    g_pEffectPositionNormal->SetWorld(scale * trans);

    // Draw
    g_pTeapot->Draw(g_pEffectPositionNormal, g_pInputLayoutPositionNormal);
}

// Draw a simple triangle using custom shaders (g_pEffect)
void DrawTriangle(ID3D11DeviceContext* pd3dImmediateContext)
{
	XMMATRIX world = g_camera.GetWorldMatrix();
	XMMATRIX view  = g_camera.GetViewMatrix();
	XMMATRIX proj  = g_camera.GetProjMatrix();
	XMFLOAT4X4 mViewProj;
	XMStoreFloat4x4(&mViewProj, world * view * proj);
	g_pEffect->GetVariableByName("g_worldViewProj")->AsMatrix()->SetMatrix((float*)mViewProj.m);
	g_pEffect->GetTechniqueByIndex(0)->GetPassByIndex(0)->Apply(0, pd3dImmediateContext);
    
	pd3dImmediateContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	pd3dImmediateContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);
	pd3dImmediateContext->IASetInputLayout(nullptr);
	pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pd3dImmediateContext->Draw(3, 0);
}

void DrawPoints(ID3D11DeviceContext* pd3dImmediateContext)
{
    // Setup position/normal effect (constant variables)
    g_pEffectPositionNormal->SetEmissiveColor(Colors::Black);
    g_pEffectPositionNormal->SetSpecularColor(0.4f * Colors::White);
    g_pEffectPositionNormal->SetSpecularPower(100);
      
    std::mt19937 eng;
    std::uniform_real_distribution<float> randCol( 0.0f, 1.0f);
    std::uniform_real_distribution<float> randPos(-0.5f, 0.5f);

    for (unsigned int i = 0; i < points.size(); i++)
    {
        // Setup position/normal effect (per object variables)
        g_pEffectPositionNormal->SetDiffuseColor(0.6f * XMColorHSVToRGB(XMVectorSet(randCol(eng), 1, 1, 0)));
		if (selectedPoint == i)
		{
			XMFLOAT4 white;
			white.x = white.y = white.z = white.w = 1.0f;
			g_pEffectPositionNormal->SetDiffuseColor(XMLoadFloat4(&white));
		}
        XMMATRIX scale    = XMMatrixScaling(g_fSphereSize, g_fSphereSize, g_fSphereSize);
		Vec3 position = points[i].getPosition();
        XMMATRIX trans    = XMMatrixTranslation(position.x, position.y, position.z);
        g_pEffectPositionNormal->SetWorld(scale * trans * g_camera.GetWorldMatrix());

        // Draw
        // NOTE: The following generates one draw call per object, so performance will be bad for n>>1000 or so
        g_pSphere->Draw(g_pEffectPositionNormal, g_pInputLayoutPositionNormal);
    }
}

void DrawSprings(ID3D11DeviceContext* pd3dImmediateContext)
{
	// Setup position/color effect
    g_pEffectPositionColor->SetWorld(g_camera.GetWorldMatrix());
    
    g_pEffectPositionColor->Apply(pd3dImmediateContext);
    pd3dImmediateContext->IASetInputLayout(g_pInputLayoutPositionColor);

    // Draw
    g_pPrimitiveBatchPositionColor->Begin();

	for (unsigned int i = 0; i < springs.size(); i++)
	{
		Vec3 p1Pos = points[springs[i].getPoint1()].getPosition();
		Vec3 p2Pos = points[springs[i].getPoint2()].getPosition();
		g_pPrimitiveBatchPositionColor->DrawLine(
            VertexPositionColor(XMVectorSet(p1Pos.x, p1Pos.y, p1Pos.z, 1), Colors::Blue),
            VertexPositionColor(XMVectorSet(p2Pos.x, p2Pos.y, p2Pos.z, 1), Colors::Blue)
        );
	}

	g_pPrimitiveBatchPositionColor->End();
}

/*void DrawLines(ID3D11DeviceContext* pd3dImmediateContext)
{
	// Setup position/color effect
    g_pEffectPositionColor->SetWorld(g_camera.GetWorldMatrix());
    
    g_pEffectPositionColor->Apply(pd3dImmediateContext);
    pd3dImmediateContext->IASetInputLayout(g_pInputLayoutPositionColor);

    // Draw
    g_pPrimitiveBatchPositionColor->Begin();

	for (unsigned int i = 0; i < lines.size(); i++)
	{
		g_pPrimitiveBatchPositionColor->DrawLine(
            VertexPositionColor(XMVectorSet(lines[i].x1, lines[i].y1, lines[i].z1, 1), Colors::Blue),
            VertexPositionColor(XMVectorSet(lines[i].x2, lines[i].y2, lines[i].z2, 1), Colors::Blue)
        );
	}

	g_pPrimitiveBatchPositionColor->End();
}*/



// ============================================================
// DXUT Callbacks
// ============================================================


//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
	return true;
}


//--------------------------------------------------------------------------------------
// Called right before creating a device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
	return true;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependent on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
	HRESULT hr;

    ID3D11DeviceContext* pd3dImmediateContext = DXUTGetD3D11DeviceContext();;

    std::wcout << L"Device: " << DXUTGetDeviceStats() << std::endl;
    
    // Load custom effect from "effect.fxo" (compiled "effect.fx")
	std::wstring effectPath = GetExePath() + L"effect.fxo";
	if(FAILED(hr = D3DX11CreateEffectFromFile(effectPath.c_str(), 0, pd3dDevice, &g_pEffect)))
	{
        std::wcout << L"Failed creating effect with error code " << int(hr) << std::endl;
		return hr;
	}

    // Init AntTweakBar GUI
    InitTweakBar(pd3dDevice);

    // Create DirectXTK geometric primitives for later usage
    g_pSphere = GeometricPrimitive::CreateGeoSphere(pd3dImmediateContext, 2.0f, 2, false);
    g_pTeapot = GeometricPrimitive::CreateTeapot(pd3dImmediateContext, 1.5f, 8, false);

    // Create effect, input layout and primitive batch for position/color vertices (DirectXTK)
    {
        // Effect
        g_pEffectPositionColor = new BasicEffect(pd3dDevice);
        g_pEffectPositionColor->SetVertexColorEnabled(true); // triggers usage of position/color vertices

        // Input layout
        void const* shaderByteCode;
        size_t byteCodeLength;
        g_pEffectPositionColor->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);
        
        pd3dDevice->CreateInputLayout(VertexPositionColor::InputElements,
                                      VertexPositionColor::InputElementCount,
                                      shaderByteCode, byteCodeLength,
                                      &g_pInputLayoutPositionColor);

        // Primitive batch
        g_pPrimitiveBatchPositionColor = new PrimitiveBatch<VertexPositionColor>(pd3dImmediateContext);
    }

    // Create effect, input layout and primitive batch for position/normal vertices (DirectXTK)
    {
        // Effect
        g_pEffectPositionNormal = new BasicEffect(pd3dDevice);
        g_pEffectPositionNormal->EnableDefaultLighting(); // triggers usage of position/normal vertices
        g_pEffectPositionNormal->SetPerPixelLighting(true);

        // Input layout
        void const* shaderByteCode;
        size_t byteCodeLength;
        g_pEffectPositionNormal->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

        pd3dDevice->CreateInputLayout(VertexPositionNormal::InputElements,
                                      VertexPositionNormal::InputElementCount,
                                      shaderByteCode, byteCodeLength,
                                      &g_pInputLayoutPositionNormal);

        // Primitive batch
        g_pPrimitiveBatchPositionNormal = new PrimitiveBatch<VertexPositionNormal>(pd3dImmediateContext);
    }

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
	SAFE_RELEASE(g_pEffect);
	
    TwDeleteBar(g_pTweakBar);
    g_pTweakBar = nullptr;
	TwTerminate();

    g_pSphere.reset();
    g_pTeapot.reset();
    
    SAFE_DELETE (g_pPrimitiveBatchPositionColor);
    SAFE_RELEASE(g_pInputLayoutPositionColor);
    SAFE_DELETE (g_pEffectPositionColor);

    SAFE_DELETE (g_pPrimitiveBatchPositionNormal);
    SAFE_RELEASE(g_pInputLayoutPositionNormal);
    SAFE_DELETE (g_pEffectPositionNormal);
}

//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                          const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    // Update camera parameters
	int width = pBackBufferSurfaceDesc->Width;
	int height = pBackBufferSurfaceDesc->Height;
	screenWidth = width;
	screenHeight = height;
	g_camera.SetWindow(width, height);
	g_camera.SetProjParams(XM_PI / 4.0f, float(width) / float(height), 0.1f, 100.0f);

    // Inform AntTweakBar about back buffer resolution change
  	TwWindowSize(pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height);

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
}

//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
	if(bKeyDown)
	{
		switch(nChar)
		{
            // ALT+RETURN: toggle fullscreen
			case VK_RETURN :
			{
				if(bAltDown) DXUTToggleFullScreen();
				break;
			}
            // F8: Take screenshot
			case VK_F8:
			{
                // Save current render target as png
                static int nr = 0;
				std::wstringstream ss;
				ss << L"Screenshot" << std::setfill(L'0') << std::setw(4) << nr++ << L".png";

                ID3D11Resource* pTex2D = nullptr;
                DXUTGetD3D11RenderTargetView()->GetResource(&pTex2D);
                SaveWICTextureToFile(DXUTGetD3D11DeviceContext(), pTex2D, GUID_ContainerFormatPng, ss.str().c_str());
                SAFE_RELEASE(pTex2D);

                std::wcout << L"Screenshot written to " << ss.str() << std::endl;
				break;
			}
			default : return;
		}
	}
}


//--------------------------------------------------------------------------------------
// Handle mouse button presses
//--------------------------------------------------------------------------------------
void CALLBACK OnMouse( bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown,
                       bool bSideButton1Down, bool bSideButton2Down, int nMouseWheelDelta,
                       int xPos, int yPos, void* pUserContext )
{
    // Track mouse movement if left mouse key is pressed
    {
        static int xPosSave = 0, yPosSave = 0;
		static bool leftMousePressed = false;

        if (bLeftButtonDown && !leftMousePressed)
        {
			leftMousePressed = true;
            // Accumulate deltas in g_viMouseDelta
           /* g_viMouseDelta.x += xPos - xPosSave;
            g_viMouseDelta.y += yPos - yPosSave;
        }

		if (bRightButtonDown)
		{*/
			printf("qStart: %d, %d\n", xPos, yPos);
			float dx = tanFOV * (2 * ((float)xPos)/screenWidth - 1.0f)*(((float)screenWidth)/screenHeight);
			float dy = tanFOV * (1.0f - 2 * ((float)yPos)/screenHeight);
			float nearPlane = 0.1f;
			float farPlane = 100.0f;
			XMFLOAT3 p1Float(dx * nearPlane, dy * nearPlane, nearPlane);
			XMFLOAT3 p2Float(dx * farPlane, dy * farPlane, farPlane);
			XMVECTOR p1Vector = XMLoadFloat3(&p1Float);
			XMVECTOR p2Vector = XMLoadFloat3(&p2Float);
			XMMATRIX viewInv = XMMatrixInverse(nullptr, g_camera.GetViewMatrix());
			p1Vector = XMVector3Transform(p1Vector, viewInv);
			p2Vector = XMVector3Transform(p2Vector, viewInv);
			printf("%f, %f\n", dx, dy);

			/*Line l;

			l.x1 = XMVectorGetX(p1Vector);
			l.y1 = XMVectorGetY(p1Vector);
			l.z1 = XMVectorGetZ(p1Vector);

			l.x2 = XMVectorGetX(p2Vector);
			l.y2 = XMVectorGetY(p2Vector);
			l.z2 = XMVectorGetZ(p2Vector);

			lines.push_back(l);*/

			int currentMinSphere = -1;
			float currentMinDistance = 100.0f;

			for (unsigned int i = 0 ; i < points.size(); i++)
			{
				Vec3 position = points[i].getPosition();
				
				XMMATRIX trans    = XMMatrixTranslation(position.x, position.y, position.z);
				XMMATRIX objectMatrix = trans * g_camera.GetWorldMatrix();
				objectMatrix = XMMatrixInverse(nullptr, objectMatrix);
				XMVECTOR p1VectorObject = XMVector3Transform(p1Vector, objectMatrix);
				XMVECTOR p2VectorObject = XMVector3Transform(p2Vector, objectMatrix);
				Vec3 p1Vec3;
				p1Vec3.x = XMVectorGetX(p1VectorObject);
				p1Vec3.y = XMVectorGetY(p1VectorObject);
				p1Vec3.z = XMVectorGetZ(p1VectorObject);
				Vec3 p2Vec3;
				p2Vec3.x = XMVectorGetX(p2VectorObject);
				p2Vec3.y = XMVectorGetY(p2VectorObject);
				p2Vec3.z = XMVectorGetZ(p2VectorObject);

				Vec3 d = p2Vec3 - p1Vec3;
				d.normalize();

				float a = d * d;
				float b = 2 * d * p1Vec3;
				float c = p1Vec3 * p1Vec3 - 0.05f * 0.05f;

				float disc = b * b - 4 * a * c;
				printf("disc: %f\n", disc);
				if (disc >= 0)
				{
					float discSqrt = sqrtf(disc);
					float q;
					if (b < 0)
						q = (-b - discSqrt) / 2.0f;
					else
						q = (-b + discSqrt) / 2.0f;

					float t0 = q / a;
					float t1 = c / q;

					if (t0 > t1)
					{
						float tmp = t0;
						t0 = t1;
						t1 = tmp;
					}

					printf("%d: t0: %d, t1: %d\n", i, t0, t1);

					if (t1 > 0 && t0 < 0)
					{
						if (t1 < currentMinDistance)
						{
							currentMinDistance = t1;
							currentMinSphere = i;
						}
					}
					if (t1 > 0 && t0 > 0)
					{
						if (t0 < currentMinDistance)
						{
							currentMinDistance = t0;
							currentMinSphere = i;
						}
					}
				}
			}


			if (currentMinSphere != -1)
			{
				if (addSpringMode && selectedPoint != currentMinSphere)
				{
					Spring s(&points, selectedPoint, currentMinSphere, stiffness, initialLength);
					springs.push_back(s);
					addSpringMode = false;
				}
				printf("Sphere: %d\n", currentMinSphere);
				selectedPoint = currentMinSphere;
				selectedPointDepth = currentMinDistance;
				isStatic = points[selectedPoint].getIsFixed();
				mass = points[selectedPoint].getMass();
			}
			else
			{
				selectedPoint = -1;
				/*isStatic = false;
				mass = 0.0f;*/
			}

			
			
		}

		if (bLeftButtonDown && leftMousePressed)
		{
			if (selectedPoint != -1)
			{
				float dx = tanFOV * (2 * (((float)screenWidth)/2 + 1)/screenWidth - 1.0f)*(((float)screenWidth)/screenHeight);
				float dy = tanFOV * (2 * (((float)screenHeight)/2 + 1)/screenHeight - 1.0f);
				//float depth;
				printf("depth: %f\n", selectedPointDepth);
				XMFLOAT3 relXFloat(dx * selectedPointDepth, 0, 0);
				XMFLOAT3 relYFloat(0, dy * selectedPointDepth, 0);
				XMVECTOR relXVector = XMLoadFloat3(&relXFloat);
				XMVECTOR relYVector = XMLoadFloat3(&relYFloat);
				XMMATRIX viewInv = XMMatrixInverse(nullptr, g_camera.GetViewMatrix());
				relXVector = XMVector3TransformNormal(relXVector, viewInv);
				relYVector = XMVector3TransformNormal(relYVector, viewInv);
				Vec3 relXVec3(XMVectorGetX(relXVector), XMVectorGetY(relXVector), XMVectorGetZ(relXVector));
				Vec3 relYVec3(XMVectorGetX(relYVector), XMVectorGetY(relYVector), XMVectorGetZ(relYVector));
				/*relXVec3.normalize();
				relYVec3.normalize();
				relXVec3 /= 100;
				relYVec3 /= 100;*/
				Vec3 deltaPosition = (xPos - xPosSave) * relXVec3 + (yPos - yPosSave) * (-relYVec3);

				/*XMFLOAT3 relXf, relYf;
				relXf.x = 1.0f;
				relXf.y = 0.0f;
				relXf.z = 0.0f;
				relYf.x = 0.0f;
				relYf.y = 1.0f;
				relYf.z = 0.0f;
				XMVECTOR relX = XMLoadFloat3(&relXf);
				XMVECTOR relY = XMLoadFloat3(&relYf);
				relX = XMVector3TransformNormal(relX, viewInv);
				relY = XMVector3TransformNormal(relY, viewInv);
				Vec3 relXv(XMVectorGetX(relX), XMVectorGetY(relX), XMVectorGetZ(relX));
				Vec3 relYv(XMVectorGetX(relY), XMVectorGetY(relY), XMVectorGetZ(relY));
				relXv.normalize();
				relYv.normalize();
				relXv /= 100;
				relYv /= 100;*/
				/*Vec3 relXv(0.01f, 0, 0);
				Vec3 relYv(0, 0.01f, 0);*/
				//Vec3 deltaPosition = (xPos - xPosSave) * relXv + (yPos - yPosSave) * (-relYv);
				points[selectedPoint].translate(deltaPosition.x, deltaPosition.y, deltaPosition.z);
			}
		}

		if (!bLeftButtonDown)
			leftMousePressed = false;
		
        xPosSave = xPos;
        yPosSave = yPos;
    }
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                          bool* pbNoFurtherProcessing, void* pUserContext )
{
	if (uMsg == WM_KEYDOWN)
	{
		switch (wParam)
		{
			case 0x57:
			{
				Point p(0, 0, 0);
				p.setTimeStep(timeStep);
				p.setMass(mass);
				points.push_back(p);
				break;
			}
			case 0x45:
				if (selectedPoint != -1)
				{
					addSpringMode = true;
				}
				break;
		}
	}
	

    // Send message to AntTweakbar first
    if (TwEventWin(hWnd, uMsg, wParam, lParam))
    {
        *pbNoFurtherProcessing = true;
        return 0;
    }

    // If message not processed yet, send to camera
	if(g_camera.HandleMessages(hWnd,uMsg,wParam,lParam))
    {
        *pbNoFurtherProcessing = true;
		return 0;
    }

	return 0;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double dTime, float fElapsedTime, void* pUserContext )
{
	UpdateWindowTitle(L"PhysicsDemo");

    // Move camera
    g_camera.FrameMove(fElapsedTime);

    // Update effects with new view + proj transformations
    g_pEffectPositionColor->SetView       (g_camera.GetViewMatrix());
    g_pEffectPositionColor->SetProjection (g_camera.GetProjMatrix());

    g_pEffectPositionNormal->SetView      (g_camera.GetViewMatrix());
    g_pEffectPositionNormal->SetProjection(g_camera.GetProjMatrix());

    // Apply accumulated mouse deltas to g_vfMovableObjectPos (move along cameras view plane)
    if (g_viMouseDelta.x != 0 || g_viMouseDelta.y != 0)
    {
        // Calcuate camera directions in world space
        XMMATRIX viewInv = XMMatrixInverse(nullptr, g_camera.GetViewMatrix());
        XMVECTOR camRightWorld = XMVector3TransformNormal(g_XMIdentityR0, viewInv);
        XMVECTOR camUpWorld    = XMVector3TransformNormal(g_XMIdentityR1, viewInv);

        // Add accumulated mouse deltas to movable object pos
        XMVECTOR vMovableObjectPos = XMLoadFloat3(&g_vfMovableObjectPos);

        float speedScale = 0.001f;
        vMovableObjectPos = XMVectorAdd(vMovableObjectPos,  speedScale * (float)g_viMouseDelta.x * camRightWorld);
        vMovableObjectPos = XMVectorAdd(vMovableObjectPos, -speedScale * (float)g_viMouseDelta.y * camUpWorld);

        XMStoreFloat3(&g_vfMovableObjectPos, vMovableObjectPos);
        
        // Reset accumulated mouse deltas
        g_viMouseDelta = XMINT2(0,0);
    }
}

//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext,
                                  double fTime, float fElapsedTime, void* pUserContext )
{
	static float timeAcc = 0;

	if (selectedPoint != -1)
	{
		points[selectedPoint].setIsFixed(isStatic);
		points[selectedPoint].setMass(mass);
	}

	if (isSimulating)
	{
		timeAcc += fElapsedTime;
		while (timeAcc >= timeStep)
		{
			/*for (unsigned int i = 0; i < points.size(); i++)
				points[i].clearForce();
			for (unsigned int i = 0; i < springs.size(); i++)
				springs[i].addElasticForces();
			for (unsigned int i = 0; i < points.size(); i++)
				points[i].stepEuler();*/

			for (unsigned int i = 0; i < points.size(); i++)
				points[i].clearForce();
			for (unsigned int i = 0; i < springs.size(); i++)
				springs[i].addElasticForces();
			for (unsigned int i = 0; i < points.size(); i++)
				points[i].stepMidPoint1();
			for (unsigned int i = 0; i < points.size(); i++)
				points[i].clearForce();
			for (unsigned int i = 0; i < springs.size(); i++)
				springs[i].addElasticForcesMidPoint();
			for (unsigned int i = 0; i < points.size(); i++)
				points[i].stepMidPoint2();

			timeAcc -= timeStep;


		}
	}

	// Clear render target and depth stencil
	float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
	ID3D11DepthStencilView* pDSV = DXUTGetD3D11DepthStencilView();
	pd3dImmediateContext->ClearRenderTargetView( pRTV, ClearColor );
	pd3dImmediateContext->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0f, 0 );

    // Draw floor
    DrawFloor(pd3dImmediateContext);

    // Draw axis box
    DrawBoundingBox(pd3dImmediateContext);

	DrawPoints(pd3dImmediateContext);

	DrawSprings(pd3dImmediateContext);

	//DrawLines(pd3dImmediateContext);

    // Draw speheres
    if (g_bDrawSpheres) DrawSomeRandomObjects(pd3dImmediateContext);

    // Draw movable teapot
    if (g_bDrawTeapot) DrawMovableTeapot(pd3dImmediateContext);

    // Draw simple triangle
    if (g_bDrawTriangle) DrawTriangle(pd3dImmediateContext);

    // Draw GUI
    TwDraw();
}

//--------------------------------------------------------------------------------------
// Initialize everything and go into a render loop
//--------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	screenWidth = 1280;
	screenHeight = 960;

	//tanFOV = tanf(XM_PI / 4.0f);
	tanFOV = tanf(XM_PI / 8.0f);
	printf("Tan: %f\n", tanFOV);

	Point p1(0, 0, 0);
	p1.setTimeStep(timeStep);
	points.push_back(p1);

	Point p2(-0.1f, 0.5f, 0.2f);
	p2.setTimeStep(timeStep);
	p2.setIsFixed(true);
	points.push_back(p2);

	Point p3(0.3f, 0.0f, 0.0f);
	p3.setTimeStep(timeStep);
	points.push_back(p3);

	Spring s(&points, 0, 1, 46.0f, 0.5f);
	springs.push_back(s);

	Spring s2(&points, 0, 2, 46.0f, 0.5f);
	springs.push_back(s2);

	Spring s3(&points, 1, 2, 46.0f, 0.5f);
	springs.push_back(s3);

#if defined(DEBUG) | defined(_DEBUG)
	// Enable run-time memory check for debug builds.
	// (on program exit, memory leaks are printed to Visual Studio's Output console)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

#ifdef _DEBUG
	std::wcout << L"---- DEBUG BUILD ----\n\n";
#endif

	// Set general DXUT callbacks
	DXUTSetCallbackMsgProc( MsgProc );
	DXUTSetCallbackMouse( OnMouse, true );
	DXUTSetCallbackKeyboard( OnKeyboard );

	DXUTSetCallbackFrameMove( OnFrameMove );
	DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );

	// Set the D3D11 DXUT callbacks
	DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
	DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
	DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
	DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );
	DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
	DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );

	

    // Init camera
 	XMFLOAT3 eye(0.0f, 0.0f, -2.0f);
	XMFLOAT3 lookAt(0.0f, 0.0f, 0.0f);
	g_camera.SetViewParams(XMLoadFloat3(&eye), XMLoadFloat3(&lookAt));
    g_camera.SetButtonMasks(MOUSE_MIDDLE_BUTTON, MOUSE_WHEEL, MOUSE_RIGHT_BUTTON);
	//g_camera.SetButtonMasks(MOUSE_WHEEL, MOUSE_RIGHT_BUTTON);

	

    // Init DXUT and create device
	DXUTInit( true, true, NULL ); // Parse the command line, show msgboxes on error, no extra command line params
	//DXUTSetIsInGammaCorrectMode( false ); // true by default (SRGB backbuffer), disable to force a RGB backbuffer
	DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
	DXUTCreateWindow( L"PhysicsDemo" );
	DXUTCreateDevice( D3D_FEATURE_LEVEL_10_0, true, 1280, 960 );
	DXUTMainLoop(); // Enter into the DXUT render loop

	DXUTShutdown(); // Shuts down DXUT (includes calls to OnD3D11ReleasingSwapChain() and OnD3D11DestroyDevice())

	return DXUTGetExitCode();
}
