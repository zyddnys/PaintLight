#pragma once

#include "DXUT.h"
#include <tuple>

class InputHelper
{
private:
	int m_mouseX, m_mouseY;
    UINT m_screenWidth, m_screenHeight;
public:
	LRESULT HandleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
        if (uMsg == WM_MOUSEMOVE)
        {
            m_mouseX = (short)LOWORD(lParam);
            m_mouseY = (short)HIWORD(lParam);
        }
        return S_OK;
	}
    void OnSwapChainResized(UINT width, UINT height)
    {
        m_screenWidth = width;
        m_screenHeight = height;
    }
public:
    std::tuple<int, int> GetMousePosition()
    {
        return { m_mouseX, m_mouseY };
    }
    std::tuple<float, float> GetMousePositionRelative()
    {
        float const mouseX{ static_cast<float>(m_mouseX) / static_cast<float>(m_screenWidth - 1) };
        float const mouseY{ static_cast<float>(m_mouseY) / static_cast<float>(m_screenHeight - 1) };
        return { (mouseX - .5f) * 2.0f, (mouseY - .5f) * -2.0f };
    }
};
