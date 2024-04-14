#include <keygen.h>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    // Class creation and registration.
    WNDCLASS wc = { };

    wc.lpfnWndProc = KeygenProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = KEYGEN_CLASS;
    wc.hIcon = LoadIcon(NULL, IDI_SHIELD);

    if (!RegisterClass(&wc))
    {
        showError(L"wWinMain::RegisterClass");
        return -1;
    }

    // Create the window.
    HWND hwnd;

    hwnd = CreateWindow(
        KEYGEN_CLASS, L"KeyGen",
        WS_OVERLAPPED | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT,
        KEYGEN_WIDTH, KEYGEN_HEIGHT,
        NULL, NULL, hInstance, NULL
    );
    if (hwnd == NULL)
    {
        showError(L"wWinMain::CreateWindow");
        return -1;
    }

    ShowWindow(hwnd, nCmdShow);

    // Run the message loop.
    MSG msg = { };

    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK KeygenProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        if (addMenu(hwnd) < 0 || addControls(hwnd) < 0)
        {
            showError(L"KeygenProc::GUI");
            DestroyWindow(hwnd);
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        
        }

        return 0;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

int addControls(HWND hwnd)
{
    if (!CreateWindow(
        L"STATIC", L"Username:",
        WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET,
        (KEYGEN_WIDTH) / 3,
        ELEMENT_HEIGHT,
        hwnd, NULL, NULL, NULL
    ))
    {
        showError(L"addLoginControls::STATIC::USERNAME");
        return -1;
    }

    if (!CreateWindow(
        L"EDIT", L"",
        ES_AUTOHSCROLL | WS_BORDER | WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET + (KEYGEN_WIDTH) / 3,
        ELEMENT_OFFSET,
        (KEYGEN_WIDTH - ELEMENT_OFFSET) / 2,
        ELEMENT_HEIGHT,
        hwnd, (HMENU)CID_USERNAME, NULL, NULL
    ))
    {
        showError(L"addLoginControls::EDIT::USERNAME");
        return -1;
    }

    if (!CreateWindow(
        L"STATIC", L"Password:",
        WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET * 3,
        (KEYGEN_WIDTH) / 3,
        ELEMENT_HEIGHT,
        hwnd, NULL, NULL, NULL
    ))
    {
        showError(L"addLoginControls::STATIC::PASSWORD");
        return -1;
    }

    if (!CreateWindow(
        L"EDIT", L"",
        ES_READONLY | WS_BORDER | WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET + (KEYGEN_WIDTH) / 3,
        ELEMENT_OFFSET * 3,
        (KEYGEN_WIDTH - ELEMENT_OFFSET) / 2,
        ELEMENT_HEIGHT,
        hwnd, (HMENU)CID_PASSWORD, NULL, NULL
    ))
    {
        showError(L"addLoginControls::EDIT::PASSWORD");
        return -1;
    }

    if (!CreateWindow(
        L"BUTTON", L"Generate",
        WS_BORDER | WS_CHILD | WS_VISIBLE,
        (KEYGEN_WIDTH - ELEMENT_OFFSET * 2) / 2 - BUTTON_WIDTH - ELEMENT_OFFSET,
        KEYGEN_HEIGHT - ELEMENT_OFFSET * 9,
        BUTTON_WIDTH,
        ELEMENT_HEIGHT,
        hwnd, (HMENU)CID_GENERATE, NULL, NULL
    ))
    {
        showError(L"addLoginControls::BUTTON::GENERATE");
        return -1;
    }

    if (!CreateWindow(
        L"BUTTON", L"Cancel",
        WS_BORDER | WS_CHILD | WS_VISIBLE,
        (KEYGEN_WIDTH - ELEMENT_OFFSET * 2) / 2 + ELEMENT_OFFSET,
        KEYGEN_HEIGHT - ELEMENT_OFFSET * 9,
        BUTTON_WIDTH,
        ELEMENT_HEIGHT,
        hwnd, (HMENU)CID_CANCEL, NULL, NULL
    ))
    {
        showError(L"addLoginControls::BUTTON::CANCEL");
        return -1;
    }

    return 0;
}

int addMenu(HWND hwnd)
{
    // Create and add menu bar.
    HMENU hMenuBar = CreateMenu();
    HMENU hMenuHelp = CreateMenu();

    if (!hMenuBar || !hMenuHelp)
    {
        showError(L"addMenu::CreateMenu");
        return -1;
    }

    // Create menu option.
    if (!AppendMenu(hMenuHelp, MF_STRING, MENU_ABOUT, L"About program"))
    {
        showError(L"addMenu::ABOUT::AppendMenu");
        return -1;
    }

    // Add created option to menu bar.
    if (!AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hMenuHelp, L"Help"))
    {
        showError(L"addMenu::HELP::AppendMenu");
        return -1;
    }

    if (!SetMenu(hwnd, hMenuBar))
    {
        showError(L"addMenu::SetMenu");
        return -1;
    }

    return 0;
}

void CreateNum(BYTE* pbUsername, UINT* pNum, int iUsernameSize)

{
    int iCounter = 0;

    *pNum = 0xfacc0fff;

    if (0 < iUsernameSize)
    {
        do
        {
            *pNum = (pbUsername[iCounter] ^ *pNum) << 8 | *pNum >> 0x18;
            iCounter++;
        } while (iCounter < iUsernameSize);
    }

    return;
}

void CreatePassword(UINT uNum, BYTE* pbPassword)

{
    UINT uVal;
    int iCounter = 0;
    BYTE bCharacter = 0;

    do
    {
        uVal = uNum & 0xf;
        uNum >>= 4;

        if (9 < uVal)
        {
            uVal = 9;
        }

        _itoa_s(uVal, (char*)&bCharacter, 1, 10);
        pbPassword[iCounter] = bCharacter;
        iCounter++;
    } while (iCounter < 8);

    return;
}

void showError(const std::wstring& wstrError)
{
    std::wstringstream wsstr;

    wsstr << wstrError << L". Error: " << GetLastError()
        << L" (0x" << std::hex << GetLastError() << L")" << std::endl;

    std::wstring wstr = wsstr.str();

    MessageBox(NULL, wstr.c_str(), L"Error", MB_OK);
}