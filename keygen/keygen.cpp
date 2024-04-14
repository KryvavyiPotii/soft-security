#include <keygen.h>

const wchar_t wcAboutProgram[] = L"A program to generate a password for \"Crack_me_up!.exe\" based on the entered username.";

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
        case CID_GENERATE:
            if (keygen(hwnd) < 0)
            {
                showError(L"KeygenProc::keygen");
            }
            break;

        case CID_CANCEL:
            DestroyWindow(hwnd);
            break;

        case MENU_ABOUT:
            MessageBox(
                hwnd,
                wcAboutProgram,
                L"About program",
                MB_OK | MB_ICONINFORMATION
            );
            break;
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
        ELEMENT_OFFSET * 4,
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
        ES_READONLY | WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET + (KEYGEN_WIDTH) / 3,
        ELEMENT_OFFSET * 4,
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

int keygen(HWND hwnd)
{
    // Get username.
    BYTE* pbUsername;
    DWORD dwLength;

    {
        // Get handle to username field.
        HWND hwndUsername = GetDlgItem(hwnd, CID_USERNAME);

        if (hwndUsername == NULL)
        {
            showError(L"keygen::GetDlgItem::USERNAME");
            return -1;
        }

        // Get username length.
        dwLength = GetWindowTextLengthA(hwndUsername);

        if (!dwLength)
        {
            if (!GetLastError())
            {
                MessageBox(
                    NULL,
                    L"Username is empty.",
                    L"Alert",
                    MB_OK | MB_ICONWARNING
                );
            }
            else
            {
                showError(L"keygen::GetWindowTextLength::USERNAME");
            }
            return -1;
        }

        // Resize buffer.
        pbUsername = new BYTE[dwLength + 2];
        pbUsername[dwLength + 1] = 0;

        int iSize = GetWindowTextA(hwndUsername, (char*)pbUsername, dwLength + 1);

        if (!iSize && GetLastError())
        {
            showError(L"identify::GetWindowText::USERNAME");
            return -1;
        }
    }

    // DEBUG
    // MessageBoxA(NULL, (char*)pbUsername, "Username", 0);

    UINT uNumber;

    deriveNumber(pbUsername, &uNumber, dwLength + 1);

    // Cleanup.
    delete[] pbUsername;

    // DEBUG
    //showValue(L"uNumber", uNumber);

    BYTE* pbPassword = new BYTE[9];

    createPassword(uNumber, pbPassword);

    // Add null byte.
    pbPassword[8] = 0;

    // DEBUG
    // MessageBoxA(NULL, (char*)pbPassword, "Password", 0);

    // Show output.
    {
        // Get handle to password field.
        HWND hwndPassword = GetDlgItem(hwnd, CID_PASSWORD);

        if (hwndPassword == NULL)
        {
            showError(L"keygen::GetDlgItem::PASSWORD");
            return -1;
        }

        if (!SetWindowTextA(hwndPassword, (char*)pbPassword))
        {
            showError(L"keygen::SetWindowTextA::PASSWORD");
            return -1;
        }
    }

    // Cleanup.
    delete[] pbPassword;

    return 0;
}

void deriveNumber(BYTE* pbUsername, UINT* puNumber, int iUsernameSize)

{
    int iCounter = 0;

    *puNumber = 0xfacc0fff;

    if (0 < iUsernameSize)
    {
        do
        {
            *puNumber = (pbUsername[iCounter] ^ *puNumber) << 8 | *puNumber >> 0x18;
            iCounter++;
        } while (iCounter < iUsernameSize);
    }
}

void createPassword(UINT uNumber, BYTE* pbPassword)
{
    int iCounter = 0;
    BYTE bCharacter = 0;

    do
    {
        // Get a nibble.
        UINT uNibble = uNumber & 0xf;

        // Go to the next nibble.
        uNumber >>= 4;

        // Allow only numbers between 0 and 9 inclusively.
        if (uNibble > 9)
        {
            uNibble = 9;
        }

        // Convert and add the digit to password.
        pbPassword[iCounter] = uNibble + 0x30;

        iCounter++;
    } while (iCounter < 8);
}

void showError(const std::wstring& wstrError)
{
    std::wstringstream wsstr;

    wsstr << wstrError << L". Error: " << GetLastError()
        << L" (0x" << std::hex << GetLastError() << L")" << std::endl;

    std::wstring wstr = wsstr.str();

    MessageBox(NULL, wstr.c_str(), L"Error", MB_OK);
}

void showValue(const std::wstring& wstrName, auto aValue)
{
    std::wstringstream wssValue;

    wssValue << aValue << L" (0x" << std::hex << aValue << L")" << std::endl;

    std::wstring wsValue = wssValue.str();

    MessageBox(NULL, wsValue.c_str(), wstrName.c_str(), 0);
}
