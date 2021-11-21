﻿/*  This file is part of ELEKTRONIKA-IM01.
    ELEKTRONIKA-IM01 is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.
    ELEKTRONIKA-IM01 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License along with
ELEKTRONIKA-IM01. If not, see <http://www.gnu.org/licenses/>. */

// KeyboardView.cpp

#include "stdafx.h"
#include "Main.h"
#include "Views.h"
#include "Emulator.h"

//////////////////////////////////////////////////////////////////////


#define COLOR_BK_BACKGROUND RGB(200,200,200)


HWND g_hwndKeyboard = (HWND) INVALID_HANDLE_VALUE;  // Keyboard View window handle

int m_nKeyboardBitmapLeft = 0;
int m_nKeyboardBitmapTop = 0;
BYTE m_nKeyboardKeyPressed = 0;  // BK scan-code for the key pressed, or 0

void KeyboardView_OnDraw(HDC hdc);
int KeyboardView_GetKeyByPoint(int x, int y);
void Keyboard_DrawKey(HDC hdc, BYTE keyscan);


//////////////////////////////////////////////////////////////////////

const int KEYBOARD_KEYS_ARRAY_WIDTH = 5;

// Keyboard key mapping to bitmap for BK0010-01 keyboard
const WORD m_arrKeyboardKeys[] =
{
    /* x1, y1   w, h      code */
    58,  144,  28, 23,    0001, // h 8
    96,  144,  28, 23,    0003, // Фиг
    134, 144,  28, 23,    0213, // НП
    58,  188,  28, 23,    0026, // g 7
    96,  188,  28, 23,    0027, // И
    134, 188,  28, 23,    0202, // СД
    58,  232,  28, 23,    0204, // f 6
    96,  232,  28, 23,    0200, // Empty circle
    134, 232,  28, 23,    0014, // Black circle
    58,  276,  28, 23,    0007, // e 5
    96,  276,  28, 23,    0006, // РЗ
    134, 276,  28, 23,    0073, // Three lines
    58,  320,  28, 23,    0061, // d 4
    96,  320,  28, 23,    0062, // N
    134, 320,  28, 23,    0063, // Arrow back
    58,  364,  28, 23,    0064, // c 3
    96,  364,  28, 23,    0065, // Вар
    134, 364,  28, 23,    0066, // ПХ
    58,  408,  28, 23,    0067, // b 2
    96,  408,  28, 23,    0070, // ?
    134, 408,  28, 23,    0071, // СИ
    58,  452,  28, 23,    0060, // a 1
    96,  452,  28, 23,    0055, // Arrow up
    134, 452,  28, 23,    0057, // Arrow down
};
const int m_nKeyboardKeysCount = sizeof(m_arrKeyboardKeys) / sizeof(WORD) / KEYBOARD_KEYS_ARRAY_WIDTH;


//////////////////////////////////////////////////////////////////////


void KeyboardView_RegisterClass()
{
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = KeyboardViewWndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = g_hInst;
    wcex.hIcon          = NULL;
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_BTNFACE + 1);
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = CLASSNAME_KEYBOARDVIEW;
    wcex.hIconSm        = NULL;

    RegisterClassEx(&wcex);
}

void KeyboardView_Init()
{
}

void KeyboardView_Done()
{
}

void KeyboardView_Update()
{
    ::InvalidateRect(g_hwndKeyboard, NULL, TRUE);
}

void KeyboardView_Create(HWND hwndParent, int x, int y, int width, int height)
{
    ASSERT(hwndParent != NULL);

    g_hwndKeyboard = CreateWindow(
            CLASSNAME_KEYBOARDVIEW, NULL,
            WS_CHILD | WS_VISIBLE,
            x, y, width, height,
            hwndParent, NULL, g_hInst, NULL);
}

LRESULT CALLBACK KeyboardViewWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            KeyboardView_OnDraw(hdc);

            EndPaint(hWnd, &ps);
        }
        break;
    case WM_SETCURSOR:
        {
            POINT ptCursor;  ::GetCursorPos(&ptCursor);
            ::ScreenToClient(g_hwndKeyboard, &ptCursor);
            int keyindex = KeyboardView_GetKeyByPoint(ptCursor.x, ptCursor.y);
            LPCTSTR cursor = (keyindex == -1) ? IDC_ARROW : IDC_HAND;
            ::SetCursor(::LoadCursor(NULL, cursor));
        }
        return (LRESULT)TRUE;
    case WM_LBUTTONDOWN:
        {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            WORD fwkeys = (WORD)wParam;

            int keyindex = KeyboardView_GetKeyByPoint(x, y);
            if (keyindex == -1) break;
            BYTE keyscan = (BYTE) m_arrKeyboardKeys[keyindex * KEYBOARD_KEYS_ARRAY_WIDTH + 4];
            if (keyscan == 0) break;

            BOOL okShift = ((fwkeys & MK_SHIFT) != 0);
            if (okShift && keyscan >= 0100 && keyscan <= 0137)
                keyscan += 040;
            else if (okShift && keyscan >= 0060 && keyscan <= 0077)
                keyscan -= 020;

            // Fire keydown event and capture mouse
            Emulator_KeyEvent(keyscan, TRUE);
            ::SetCapture(g_hwndKeyboard);

            // Draw focus frame for the key pressed
            HDC hdc = ::GetDC(g_hwndKeyboard);
            Keyboard_DrawKey(hdc, keyscan);
            VERIFY(::ReleaseDC(g_hwndKeyboard, hdc));

            // Remember key pressed
            m_nKeyboardKeyPressed = keyscan;
        }
        break;
    case WM_LBUTTONUP:
        if (m_nKeyboardKeyPressed != 0)
        {
            // Fire keyup event and release mouse
            Emulator_KeyEvent(m_nKeyboardKeyPressed, FALSE);
            ::ReleaseCapture();

            // Draw focus frame for the released key
            HDC hdc = ::GetDC(g_hwndKeyboard);
            Keyboard_DrawKey(hdc, m_nKeyboardKeyPressed);
            VERIFY(::ReleaseDC(g_hwndKeyboard, hdc));

            m_nKeyboardKeyPressed = 0;
        }
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return (LRESULT)FALSE;
}

void KeyboardView_OnDraw(HDC hdc)
{
    RECT rc;  ::GetClientRect(g_hwndKeyboard, &rc);

    // Keyboard background
    HBRUSH hBkBrush = ::CreateSolidBrush(COLOR_BK_BACKGROUND);
    HGDIOBJ hOldBrush = ::SelectObject(hdc, hBkBrush);
    ::PatBlt(hdc, 0, 0, rc.right, rc.bottom, PATCOPY);
    ::SelectObject(hdc, hOldBrush);
    VERIFY(::DeleteObject(hBkBrush));

    if (m_nKeyboardKeyPressed != 0)
        Keyboard_DrawKey(hdc, m_nKeyboardKeyPressed);

    //DEBUG: Show key mappings
    for (int i = 0; i < m_nKeyboardKeysCount; i++)
    {
        RECT rcKey;
        rcKey.left = m_nKeyboardBitmapLeft + m_arrKeyboardKeys[i * KEYBOARD_KEYS_ARRAY_WIDTH];
        rcKey.top = m_nKeyboardBitmapTop + m_arrKeyboardKeys[i * KEYBOARD_KEYS_ARRAY_WIDTH + 1];
        rcKey.right = rcKey.left + m_arrKeyboardKeys[i * KEYBOARD_KEYS_ARRAY_WIDTH + 2];
        rcKey.bottom = rcKey.top + m_arrKeyboardKeys[i * KEYBOARD_KEYS_ARRAY_WIDTH + 3];
        ::DrawFocusRect(hdc, &rcKey);
    }
}

// Returns: index of key under the cursor position, or -1 if not found
int KeyboardView_GetKeyByPoint(int x, int y)
{
    for (int i = 0; i < m_nKeyboardKeysCount; i++)
    {
        RECT rcKey;
        rcKey.left = m_nKeyboardBitmapLeft + m_arrKeyboardKeys[i * KEYBOARD_KEYS_ARRAY_WIDTH];
        rcKey.top = m_nKeyboardBitmapTop + m_arrKeyboardKeys[i * KEYBOARD_KEYS_ARRAY_WIDTH + 1];
        rcKey.right = rcKey.left + m_arrKeyboardKeys[i * KEYBOARD_KEYS_ARRAY_WIDTH + 2];
        rcKey.bottom = rcKey.top + m_arrKeyboardKeys[i * KEYBOARD_KEYS_ARRAY_WIDTH + 3];

        if (x >= rcKey.left && x < rcKey.right && y >= rcKey.top && y < rcKey.bottom)
        {
            return i;
        }
    }
    return -1;
}

void Keyboard_DrawKey(HDC hdc, BYTE keyscan)
{
    for (int i = 0; i < m_nKeyboardKeysCount; i++)
        if (keyscan == m_arrKeyboardKeys[i * KEYBOARD_KEYS_ARRAY_WIDTH + 4])
        {
            RECT rcKey;
            rcKey.left = m_nKeyboardBitmapLeft + m_arrKeyboardKeys[i * KEYBOARD_KEYS_ARRAY_WIDTH];
            rcKey.top = m_nKeyboardBitmapTop + m_arrKeyboardKeys[i * KEYBOARD_KEYS_ARRAY_WIDTH + 1];
            rcKey.right = rcKey.left + m_arrKeyboardKeys[i * KEYBOARD_KEYS_ARRAY_WIDTH + 2];
            rcKey.bottom = rcKey.top + m_arrKeyboardKeys[i * KEYBOARD_KEYS_ARRAY_WIDTH + 3];
            ::DrawFocusRect(hdc, &rcKey);
        }
}


//////////////////////////////////////////////////////////////////////
