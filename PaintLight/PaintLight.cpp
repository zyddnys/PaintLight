//--------------------------------------------------------------------------------------
// File: EmptyProject11.cpp
//
// Empty starting point for new Direct3D 11 Win32 desktop applications
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include "DXUTgui.h"
#include "SDKmisc.h"

#include "OpenFileDialog.h"

#include "ScreenQuad.h"
#include "InputHelper.h"
#include "RGBAImage.h"

#include "NormalizeImage.h"
#include "PaintLight.h"

#pragma warning( disable : 4100 )

using namespace DirectX;

ScreenQuad  g_screenQuad;
InputHelper g_inputHelper;
NormalizeImage g_normalizeImage;

ULONG g_selectedImage;

//------------------
//   DXUT stuffs
//------------------
CDXUTDialogResourceManager  g_DialogResourceManager; // manager for shared resources of dialogs
CDXUTTextHelper            *g_pTxtHelper{ nullptr };
CDXUTDialog                 g_HUD;
CDXUTDialog                 g_SampleUI;             // dialog for sample specific controls
CDXUTComboBox              *g_DisplayImageSelectionCombo;

void CALLBACK OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl *pControl, void *pUserContext);

#define IDC_OPEN_FILE 0
#define IDC_DISPLAY_IMAGE_SEL 1

#define IDC_BLUR_RADIUS_TEXT 2
#define IDC_BLUR_RADIUS 3

#define IDC_BLUR_SIGMA_TEXT 4
#define IDC_BLUR_SIGMA 5

#define IDC_GAMMA_TEXT 6
#define IDC_GAMMA 7

#define IDC_AMBIENT_TEXT 8
#define IDC_AMBIENT 9

#define IDC_LIGHTZ_TEXT 10
#define IDC_LIGHTZ 11

#define IDC_PIXELSCALE_TEXT 12
#define IDC_PIXELSCALE 13

#define IDC_LIGHTSCALE_TEXT 14
#define IDC_LIGHTSCALE 15

#define IDC_GAMMACOR_TEXT 16
#define IDC_GAMMACOR 17

//------------------------
//   PaintLight stuffs
//------------------------
PaintLight  g_paintLight;

void InitApp();
void RenderText();

//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *AdapterInfo, UINT Output, const CD3D11EnumDeviceInfo *DeviceInfo,
                                       DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    return true;
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
    return true;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                      void* pUserContext )
{
    g_screenQuad = ScreenQuad(pd3dDevice);
    g_paintLight = PaintLight();

    auto pd3dImmediateContext{ DXUTGetD3D11DeviceContext() };
    THROW(g_DialogResourceManager.OnD3D11CreateDevice(pd3dDevice, pd3dImmediateContext));
    g_pTxtHelper = new CDXUTTextHelper(pd3dDevice, pd3dImmediateContext, &g_DialogResourceManager, 15);

    g_normalizeImage = NormalizeImage(pd3dDevice, pd3dImmediateContext);
    g_paintLight = PaintLight(pd3dDevice, pd3dImmediateContext);

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                          const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    THROW(g_DialogResourceManager.OnD3D11ResizedSwapChain(pd3dDevice, pBackBufferSurfaceDesc));

    g_HUD.SetLocation(pBackBufferSurfaceDesc->Width - 170, 0);
    g_HUD.SetSize(170, 170);
    g_SampleUI.SetLocation(pBackBufferSurfaceDesc->Width - 170, pBackBufferSurfaceDesc->Height - 300);
    g_SampleUI.SetSize(170, 300);

    g_inputHelper.OnSwapChainResized(pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height);
    return S_OK;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
    if (g_paintLight)
    {
        auto const [mouseX, mouseY] = g_inputHelper.GetMousePositionRelative();
        float lightPosX(mouseX * 0.5f * float(g_paintLight.original.width) * g_paintLight.light_scale + 0.5f * float(g_paintLight.original.width));
        float lightPosY(mouseY * 0.5f * float(g_paintLight.original.height) * g_paintLight.light_scale + 0.5f * float(g_paintLight.original.height));
        g_paintLight.light_x = -lightPosX;
        g_paintLight.light_y = lightPosY;
    }
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext,
                                  double fTime, float fElapsedTime, void* pUserContext )
{
    float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    // Clear render target and the depth stencil 
    auto pRTV = DXUTGetD3D11RenderTargetView();
    pd3dImmediateContext->ClearRenderTargetView( pRTV, blendFactor);

    auto pDSV = DXUTGetD3D11DepthStencilView();
    pd3dImmediateContext->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0, 0 );

    if (g_paintLight)
    {
        g_paintLight(pd3dDevice, pd3dImmediateContext);
        switch (g_selectedImage)
        {
        case 0:
            g_screenQuad.Draw(pd3dImmediateContext, g_paintLight.result_GPU.srv, 0.0f, 255.0f, g_paintLight.gamma_correction);
            break;
        case 1:
            g_screenQuad.Draw(pd3dImmediateContext, g_paintLight.original_GPU.srv, 0.0f, 255.0f, g_paintLight.gamma_correction);
            break;
        case 2:
            g_screenQuad.Draw(pd3dImmediateContext, g_paintLight.palette_GPU.srv, 0.0f, 255.0f, g_paintLight.gamma_correction);
            break;
        case 3:
            g_screenQuad.Draw(pd3dImmediateContext, g_paintLight.stroke_density_GPU.srv, 0.0f, 1.0f, g_paintLight.gamma_correction);
            break;
        case 4:
            g_screenQuad.Draw(pd3dImmediateContext, g_paintLight.blurred_image_GPU.srv, 0.0f, 255.0f, g_paintLight.gamma_correction);
            break;
        case 7:
            {
                g_screenQuad.Draw(pd3dImmediateContext, g_paintLight.refined_lighting_GPU.srv, 0.0f, 1.0f, g_paintLight.gamma_correction);
            }
            break;
        case 8:
            {
                g_screenQuad.Draw(pd3dImmediateContext, g_paintLight.final_lighting_GPU.srv, 0.0f, 1.0f, g_paintLight.gamma_correction);
            }
            break;
        }
    }

    g_HUD.OnRender(fElapsedTime);
    g_SampleUI.OnRender(fElapsedTime);
    RenderText();
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
    g_DialogResourceManager.OnD3D11DestroyDevice();
    DXUTGetGlobalResourceCache().OnDestroyDevice();
    g_screenQuad.Release();
    g_paintLight.Release();
    g_normalizeImage.Release();
    //g_mukyuImageGPU.Release();
    //g_gaussianBlur.Release();
    //g_imageMinMax.Release();
    //g_addScalar.Release();
    //g_mulScalar.Release();
    //g_coarseLighting.Release();
    SAFE_DELETE(g_pTxtHelper);
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                          bool* pbNoFurtherProcessing, void* pUserContext )
{
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc(hWnd, uMsg, wParam, lParam);
    if (*pbNoFurtherProcessing)
        return 0;
    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = g_HUD.MsgProc(hWnd, uMsg, wParam, lParam);
    if (*pbNoFurtherProcessing)
        return 0;
    *pbNoFurtherProcessing = g_SampleUI.MsgProc(hWnd, uMsg, wParam, lParam);
    if (*pbNoFurtherProcessing)
        return 0;
    g_inputHelper.HandleMessages(hWnd, uMsg, wParam, lParam);
    return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
}


//--------------------------------------------------------------------------------------
// Handle mouse button presses
//--------------------------------------------------------------------------------------
void CALLBACK OnMouse( bool bLeftButtonDown, bool bRightButtonDown, bool bMiddleButtonDown,
                       bool bSideButton1Down, bool bSideButton2Down, int nMouseWheelDelta,
                       int xPos, int yPos, void* pUserContext )
{
}


//--------------------------------------------------------------------------------------
// Call if device was removed.  Return true to find a new device, false to quit
//--------------------------------------------------------------------------------------
bool CALLBACK OnDeviceRemoved( void* pUserContext )
{
    return true;
}


//--------------------------------------------------------------------------------------
// Initialize everything and go into a render loop
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow )
{
    // Enable run-time memory check for debug builds.
#ifdef _DEBUG
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // DXUT will create and use the best device
    // that is available on the system depending on which D3D callbacks are set below

    // Set general DXUT callbacks
    DXUTSetCallbackFrameMove( OnFrameMove );
    DXUTSetCallbackKeyboard( OnKeyboard );
    DXUTSetCallbackMouse( OnMouse );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
    DXUTSetCallbackDeviceRemoved( OnDeviceRemoved );

    // Set the D3D11 DXUT callbacks. Remove these sets if the app doesn't need to support D3D11
    DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
    DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
    DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
    DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );
    DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
    DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );

    // Perform any application-level initialization here
    InitApp();

    DXUTInit( true, true, nullptr ); // Parse the command line, show msgboxes on error, no extra command line params
    DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
    DXUTCreateWindow( L"吹爆一喵" );

    // Only require 10-level hardware or later
    DXUTCreateDevice(D3D_FEATURE_LEVEL_11_1, true, 800, 600 );
    DXUTMainLoop(); // Enter into the DXUT ren  der loop

    // Perform any application-level cleanup here

    return DXUTGetExitCode();
}

void InitApp()
{
    g_HUD.Init(&g_DialogResourceManager);
    g_SampleUI.Init(&g_DialogResourceManager);

    g_HUD.SetCallback(OnGUIEvent); int iY = 10;

    g_HUD.AddButton(IDC_OPEN_FILE, L"Open image file", 0, iY, 170, 23);
    g_HUD.AddComboBox(IDC_DISPLAY_IMAGE_SEL, 0, iY += 26, 170, 23, VK_F10, false, &g_DisplayImageSelectionCombo);
    g_DisplayImageSelectionCombo->AddItem(L"Result", ULongToPtr(0));
    g_DisplayImageSelectionCombo->AddItem(L"Original", ULongToPtr(1));
    g_DisplayImageSelectionCombo->AddItem(L"Palette", ULongToPtr(2));
    g_DisplayImageSelectionCombo->AddItem(L"Stroke Density", ULongToPtr(3));
    g_DisplayImageSelectionCombo->AddItem(L"Blurred Image", ULongToPtr(4));
    g_DisplayImageSelectionCombo->AddItem(L"Refined Lighting", ULongToPtr(7));
    g_DisplayImageSelectionCombo->AddItem(L"Final Lighting", ULongToPtr(8));

    WCHAR desc[256] = { 0 };

    swprintf_s(desc, 255, L"Blur radius: %d ", g_paintLight.blur_width);
    g_HUD.AddStatic(IDC_BLUR_RADIUS_TEXT, desc, 10, iY + 26, 30, 10);
    g_HUD.AddSlider(IDC_BLUR_RADIUS, 10, iY += 46, 128, 15, 1, 92, g_paintLight.blur_width);

    swprintf_s(desc, 255, L"Blur sigma: %.1f ", g_paintLight.blur_sigma);
    g_HUD.AddStatic(IDC_BLUR_SIGMA_TEXT, desc, 10, iY + 26, 30, 10);
    g_HUD.AddSlider(IDC_BLUR_SIGMA, 10, iY += 46, 128, 15, 1, 500, static_cast<INT>(g_paintLight.blur_sigma * 10.0f));

    swprintf_s(desc, 255, L"Gamma: %.3f ", g_paintLight.gamma);
    g_HUD.AddStatic(IDC_GAMMA_TEXT, desc, 10, iY + 26, 30, 10);
    g_HUD.AddSlider(IDC_GAMMA, 10, iY += 46, 128, 15, 0, 1000, static_cast<INT>(g_paintLight.gamma * 100.0f));

    swprintf_s(desc, 255, L"Ambient: %.3f ", g_paintLight.ambient);
    g_HUD.AddStatic(IDC_AMBIENT_TEXT, desc, 10, iY + 26, 30, 10);
    g_HUD.AddSlider(IDC_AMBIENT, 10, iY += 46, 128, 15, 0, 100, static_cast<INT>(g_paintLight.ambient * 100.0f));
    
    swprintf_s(desc, 255, L"Light Z: %.1f ", g_paintLight.light_z);
    g_HUD.AddStatic(IDC_LIGHTZ_TEXT, desc, 10, iY + 26, 30, 10);
    g_HUD.AddSlider(IDC_LIGHTZ, 10, iY += 46, 128, 15, 0, 200, static_cast<INT>(g_paintLight.light_z * 100.0f));
    
    swprintf_s(desc, 255, L"Pixel Scale: %.3f ", g_paintLight.pixel_scale);
    g_HUD.AddStatic(IDC_PIXELSCALE_TEXT, desc, 10, iY + 26, 30, 10);
    g_HUD.AddSlider(IDC_PIXELSCALE, 10, iY += 46, 128, 15, 0, 400, static_cast<INT>(g_paintLight.pixel_scale * 100.0f));
    
    swprintf_s(desc, 255, L"Light Scale: %.1f ", g_paintLight.light_scale);
    g_HUD.AddStatic(IDC_LIGHTSCALE_TEXT, desc, 10, iY + 26, 30, 10);
    g_HUD.AddSlider(IDC_LIGHTSCALE, 10, iY += 46, 128, 15, 1, 100, static_cast<INT>(g_paintLight.light_scale));

    swprintf_s(desc, 255, L"Gamma Cor: %.2f ", g_paintLight.gamma_correction);
    g_HUD.AddStatic(IDC_GAMMACOR_TEXT, desc, 10, iY + 26, 30, 10);
    g_HUD.AddSlider(IDC_GAMMACOR, 10, iY += 46, 128, 15, 0, 200, static_cast<INT>(g_paintLight.gamma_correction * 100.0f));

    g_SampleUI.SetCallback(OnGUIEvent); iY = 10;

}

//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent(UINT nEvent, int nControlID, CDXUTControl *pControl, void *pUserContext)
{
    switch (nControlID)
    {
    case IDC_OPEN_FILE:
        {
            OutputDebugString(L"click open file");
            OpenFileDialog dialog;
            auto ret(dialog());
            if (ret.empty())
                break;
            g_paintLight.ResetWithNewImage(DXUTGetD3D11Device(), DXUTGetD3D11DeviceContext(), ret);
            MessageBox(DXUTGetHWND(), L"Please wait while the program is calculating ray intersection.", L"OpenMP disabled", MB_ICONINFORMATION | MB_OK);
            RECT rect;
            GetWindowRect(DXUTGetHWND(), std::addressof(rect));
            g_paintLight.ComputeStrokeDensityCPU(DXUTGetD3D11Device(), DXUTGetD3D11DeviceContext());
            MoveWindow(DXUTGetHWND(), rect.left, rect.top, g_paintLight.original.width, g_paintLight.original.height, FALSE);
        }
        break;
    case IDC_DISPLAY_IMAGE_SEL:
        g_selectedImage = PtrToUlong(g_DisplayImageSelectionCombo->GetSelectedData());
        break;
    case IDC_BLUR_RADIUS:
        {
            INT value = g_HUD.GetSlider(IDC_BLUR_RADIUS)->GetValue();
            value = std::min(std::max(value, 1), 92);
            WCHAR desc[256] = { 0 };
            swprintf_s(desc, L"Blur width: %d ", value);
            g_HUD.GetStatic(IDC_BLUR_RADIUS_TEXT)->SetText(desc);
            if (nEvent == EVENT_SLIDER_VALUE_CHANGED_UP)
                g_paintLight.blur_width = value;
        }
        break;
    case IDC_BLUR_SIGMA:
        {
            INT value = g_HUD.GetSlider(IDC_BLUR_SIGMA)->GetValue();
            value = std::min(std::max(value, 1), 500);
            float val(static_cast<float>(value) * 0.1f);
            WCHAR desc[256] = { 0 };
            swprintf_s(desc, L"Blur sigma: %.1f ", val);
            g_HUD.GetStatic(IDC_BLUR_SIGMA_TEXT)->SetText(desc);
            if (nEvent == EVENT_SLIDER_VALUE_CHANGED_UP)
                g_paintLight.blur_sigma = val;
        }
        break;
    case IDC_GAMMA:
    {
        INT value = g_HUD.GetSlider(IDC_GAMMA)->GetValue();
        value = std::min(std::max(value, 0), 1000);
        float val(static_cast<float>(value) * 0.01f);
        WCHAR desc[256] = { 0 };
        swprintf_s(desc, L"Gamma: %.3f ", val);
        g_HUD.GetStatic(IDC_GAMMA_TEXT)->SetText(desc);
        if (nEvent == EVENT_SLIDER_VALUE_CHANGED_UP)
            g_paintLight.gamma = val;
    }
    break;
    case IDC_AMBIENT:
    {
        INT value = g_HUD.GetSlider(IDC_AMBIENT)->GetValue();
        value = std::min(std::max(value, 0), 100);
        float val(static_cast<float>(value) * 0.01f);
        WCHAR desc[256] = { 0 };
        swprintf_s(desc, L"Ambient: %.3f ", val);
        g_HUD.GetStatic(IDC_AMBIENT_TEXT)->SetText(desc);
        if (nEvent == EVENT_SLIDER_VALUE_CHANGED_UP)
            g_paintLight.ambient = val;
    }
    break;
    case IDC_LIGHTZ:
    {
        INT value = g_HUD.GetSlider(IDC_LIGHTZ)->GetValue();
        value = std::min(std::max(value, 0), 200);
        float val(static_cast<float>(value) / 100.0f);
        WCHAR desc[256] = { 0 };
        swprintf_s(desc, L"Light Z: %.1f ", val);
        g_HUD.GetStatic(IDC_LIGHTZ_TEXT)->SetText(desc);
        if (nEvent == EVENT_SLIDER_VALUE_CHANGED_UP)
            g_paintLight.light_z = val;
    }
    break;
    case IDC_PIXELSCALE:
    {
        INT value = g_HUD.GetSlider(IDC_PIXELSCALE)->GetValue();
        value = std::min(std::max(value, 0), 400);
        float val(static_cast<float>(value) * 0.01f);
        WCHAR desc[256] = { 0 };
        swprintf_s(desc, L"Pixel Scale: %.3f ", val);
        g_HUD.GetStatic(IDC_PIXELSCALE_TEXT)->SetText(desc);
        if (nEvent == EVENT_SLIDER_VALUE_CHANGED_UP)
            g_paintLight.pixel_scale = val;
    }
    break;
    case IDC_LIGHTSCALE:
    {
        INT value = g_HUD.GetSlider(IDC_LIGHTSCALE)->GetValue();
        value = std::min(std::max(value, 1), 100);
        float val(static_cast<float>(value));
        WCHAR desc[256] = { 0 };
        swprintf_s(desc, L"Light Scale: %.3f ", val);
        g_HUD.GetStatic(IDC_LIGHTSCALE_TEXT)->SetText(desc);
        if (nEvent == EVENT_SLIDER_VALUE_CHANGED_UP)
            g_paintLight.light_scale = val;
    }
    break;
    case IDC_GAMMACOR:
    {
        INT value = g_HUD.GetSlider(IDC_GAMMACOR)->GetValue();
        value = std::min(std::max(value, 0), 200);
        float val(static_cast<float>(value) * 0.01f);
        WCHAR desc[256] = { 0 };
        swprintf_s(desc, L"Gamma Cor: %.3f ", val);
        g_HUD.GetStatic(IDC_GAMMACOR_TEXT)->SetText(desc);
        if (nEvent == EVENT_SLIDER_VALUE_CHANGED_UP)
            g_paintLight.gamma_correction = val;
    }
    break;
    }

}

void RenderText()
{
    UINT nBackBufferHeight = DXUTGetDXGIBackBufferSurfaceDesc()->Height;

    g_pTxtHelper->Begin();
    g_pTxtHelper->SetInsertionPos(2, 0);
    g_pTxtHelper->SetForegroundColor(Colors::Green);
    g_pTxtHelper->DrawTextLine(DXUTGetFrameStats(DXUTIsVsyncEnabled()));
    g_pTxtHelper->DrawTextLine(DXUTGetDeviceStats());

    auto const [mouseX, mouseY] = g_inputHelper.GetMousePositionRelative();

    wchar_t buf[256];
    swprintf_s(buf, 255, L"MouseX: %.4f, MouseY: %.4f\0", mouseX, mouseY);
    g_pTxtHelper->DrawTextLine(buf);
    swprintf_s(buf, 255, L"LightX: %.4f, LightY: %.4f\0", g_paintLight.light_x, g_paintLight.light_y);
    g_pTxtHelper->DrawTextLine(buf);
    g_pTxtHelper->End();
}
