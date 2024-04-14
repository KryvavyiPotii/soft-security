#ifndef UNICODE
#define UNICODE
#endif 

#include "app.h"

const wchar_t wcRegistryKey[] = L"Software\\Surname";
const wchar_t wcSignatureValue[] = L"Signature";
const wchar_t wcPassphraseValue[] = L"Passphrase";
const wchar_t wcCredsFilePath[] = L"creds";
const wchar_t wcBanned[] = { L'%', CREDS_DELIMITER[0] };
const wchar_t wcAboutProgram[] = L"Author: KryvavyiPotii\n\n"
    L"Task: Password must contain only latin and cyrillic letters, numbers and punctuation.";

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    // Check if app was not copied from another machine.
    {
        // Get signature value.
        BYTE* pbSignature = NULL;

        DWORD dwSize = readRegistryValue(&pbSignature, wcSignatureValue);

        if (dwSize < 0)
        {
            return -1;
        }

        // Get system info.
        std::wstring wstrInfo;

        if (getInfo(wstrInfo) < 0)
        {
            return -1;
        }

        // Verify signature.
        if (checkSignature(pbSignature, dwSize, wstrInfo.c_str()) < 0)
        {
            MessageBox(
                NULL,
                L"Program was copied from another machine.\nPress \"OK\" to exit program.",
                L"Alert",
                MB_OK | MB_ICONWARNING
            );
            return -1;
        }

        // Cleanup.
        delete[] pbSignature;
    }

    // Create file with credentials on the first run if it does not exist.
    std::error_code ec;
    
    if (!std::filesystem::exists(wcCredsFilePath, ec))
    {
        if (!ec.value())
        {
            if (createCreds() < 0)
            {
                return -1;
            }
        }
        else
        {
            showError(L"wWinMain::std::filesystem::exists");
            return -1;
        }
    }

    // Class creation and registration.
    WNDCLASS wc = { };

    wc.lpfnWndProc = LoginProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = LOGIN_CLASS;
    wc.hIcon = LoadIcon(NULL, IDI_QUESTION);

    if (!RegisterClass(&wc))
    {
        showError(L"wWinMain::RegisterClass");
        return -1;
    }

    // Create the window.
    HWND hwnd;

    hwnd = CreateWindow(
        LOGIN_CLASS, L"Login",
        WS_OVERLAPPED | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT,
        LOGIN_WIDTH, LOGIN_HEIGHT,
        NULL, NULL, hInstance, NULL
    );
    if (hwnd == NULL)
    {
        showError(L"wWinMain::CreateWindow");
        return -1;
    }

    // Register all types of windows.
    if (registerChildren(hInstance) < 0)
    {
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

int registerChildren(HINSTANCE hInstance)
{
    // Register classes for child windows.
    // Register ADMIN window class.
    WNDCLASS wcChild = { };

    ZeroMemory(&wcChild, sizeof(WNDCLASS));
    wcChild.lpfnWndProc = AdminProc;
    wcChild.hInstance = hInstance;
    wcChild.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcChild.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcChild.lpszClassName = ADMIN_CLASS;
    wcChild.hIcon = LoadIcon(NULL, IDI_SHIELD);

    if (!RegisterClass(&wcChild))
    {
        //showError(L"registerChildren()::ADMIN::RegisterClass()");
        return -1;
    }

    // Register non-privileged user window class.
    wcChild.lpfnWndProc = UserProc;
    wcChild.lpszClassName = USER_CLASS;
    wcChild.hIcon = LoadIcon(NULL, IDI_INFORMATION);

    if (!RegisterClass(&wcChild))
    {
        //showError(L"registerChildren()::USER::RegisterClass()");
        return -1;
    }

    // Register password change window class.
    wcChild.lpfnWndProc = ChpassProc;
    wcChild.lpszClassName = CHPASS_CLASS;
    wcChild.hIcon = LoadIcon(NULL, IDI_INFORMATION);

    if (!RegisterClass(&wcChild))
    {
        //showError(L"registerChildren()::CHPASS::RegisterClass()");
        return -1;
    }

    // Register user add window class.
    wcChild.lpfnWndProc = AddUserProc;
    wcChild.lpszClassName = ADD_CLASS;
    wcChild.hIcon = LoadIcon(NULL, IDI_SHIELD);

    if (!RegisterClass(&wcChild))
    {
        //showError(L"registerChildren()::ADD::RegisterClass()");
        return -1;
    }

    // Register user edit window class.
    wcChild.lpfnWndProc = EditUserProc;
    wcChild.lpszClassName = EDIT_CLASS;
    wcChild.hIcon = LoadIcon(NULL, IDI_SHIELD);

    if (!RegisterClass(&wcChild))
    {
        //showError(L"registerChildren()::EDIT::RegisterClass()");
        return -1;
    }

    return 0;
}

LRESULT CALLBACK LoginProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Create struct with username and number of login tries.
    static CurrentUser stCurrentUser;

    // Create buffer for temp file path.
    static std::wstring wstrTempPath(MAX_PATH + 1, 0);

    switch (uMsg)
    {
    case WM_CREATE:
        if (addLoginControls(hwnd) < 0)
        {
            DestroyWindow(hwnd);
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

        EndPaint(hwnd, &ps);

        return 0;
    }
    
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case LOGIN:
        {
            // Identification.
            if (identify(wstrTempPath, hwnd, &stCurrentUser) < 0)
            {
                break;
            }

            // Authentication.
            if (authenticate(wstrTempPath, hwnd, &stCurrentUser) < 0)
            {
                break;
            }

            // Authorization.
            HWND hwndUser = authorize(stCurrentUser.wstrUsername);

            if (!hwndUser)
            {
                break;
            }

            // Send temp file path to user window.
            sendData(
                hwndUser,
                hwnd,
                (BYTE*)wstrTempPath.c_str(),
                (wstrTempPath.size() + 1) * sizeof(wchar_t),
                SRID_TEMPPATH
            );

            // Send username of logged in user.
            sendData(
                hwndUser,
                hwnd,
                (BYTE*)stCurrentUser.wstrUsername.c_str(),
                (stCurrentUser.wstrUsername.size() + 1) * sizeof(wchar_t),
                SRID_USERNAME
            );

            // Hide login window.
            ShowWindow(hwnd, SW_HIDE);

            break;
        }

        case LOGIN_CLOSE:
            DestroyWindow(hwnd);
            break;
        }

        return 0;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

LRESULT CALLBACK AdminProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Get current instance handle for window creation.
    static HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(NULL);

    if (!hInstance)
    {
        showError(L"AdminProc::GetModuleHandle");
        return -1;
    }

    // Create buffer for temp file path.
    static BYTE* pbTempPath;
    static DWORD dwTempPathSize;

    switch (uMsg)
    {
    case WM_CREATE:
    {
        if (addMenu(hwnd) < 0 || addAdminControls(hwnd) < 0)
        {
            DestroyWindow(hwnd);
        }
        return 0;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

        EndPaint(hwnd, &ps);

        return 0;
    }

    case WM_DESTROY:
    {
        // Convert BYTE array to wstring for processing.
        std::wstring wstrTempPath(
            (const wchar_t*)pbTempPath,
            dwTempPathSize / sizeof(wchar_t)
        );

        // Encrypt temp file.
        if (encryptCreds(wstrTempPath) < 0)
        {
            showError(L"AdminProc::encryptCreds");
        }

        // Cleanup.
        if (pbTempPath)
        {
            delete[] pbTempPath;
        }

        PostQuitMessage(0);
        return 0;
    }

    // Receive data from owner window.
    case WM_COPYDATA:
    {
        COPYDATASTRUCT* pcds = (COPYDATASTRUCT*)lParam;

        switch (pcds->dwData)
        {
        case SRID_TEMPPATH:
            // Get buffer size.
            dwTempPathSize = pcds->cbData;

            // Allocate memory for temp file path.
            pbTempPath = new BYTE[dwTempPathSize];

            // Write data.
            memcpy_s(pbTempPath, dwTempPathSize, pcds->lpData, pcds->cbData);

            // Convert BYTE array to wstring for processing.
            std::wstring wstrTempPath(
                (const wchar_t*)pbTempPath,
                dwTempPathSize / sizeof(wchar_t)
            );

            // Create and fill user list.
            if (createListBox(hwnd, wstrTempPath) < 0)
            {
                DestroyWindow(hwnd);
            }

            break;
        }
        
        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ADMIN_ADD:
        {
            HWND hwndAdd = CreateWindow(
                ADD_CLASS, L"User add",
                WS_OVERLAPPED,
                CW_USEDEFAULT, CW_USEDEFAULT,
                ADD_WIDTH, ADD_HEIGHT,
                hwnd, NULL, hInstance, NULL
            );

            if (!hwndAdd)
            {
                showError(L"AdminProc::ADD::CreateWindow");
                break;
            }

            ShowWindow(hwndAdd, SW_SHOW);
            UpdateWindow(hwndAdd);

            // Send temp file path to user add window.
            sendData(
                hwndAdd,
                hwnd,
                pbTempPath,
                dwTempPathSize,
                SRID_TEMPPATH
            );

            // Disable ADMIN window.
            EnableWindow(hwnd, FALSE);

            break;
        }

        case ADMIN_EDIT:
        {
            HWND hwndEdit = CreateWindow(
                EDIT_CLASS, L"User edit",
                WS_OVERLAPPED,
                CW_USEDEFAULT, CW_USEDEFAULT,
                EDIT_WIDTH, EDIT_HEIGHT,
                hwnd, NULL, hInstance, NULL
            );
            if (!hwndEdit)
            {
                showError(L"AdminProc::EDIT::CreateWindow");
                break;
            }

            ShowWindow(hwndEdit, SW_SHOW);
            UpdateWindow(hwndEdit);

            // Send temp file path to user edit window.
            sendData(
                hwndEdit,
                hwnd,
                pbTempPath,
                dwTempPathSize,
                SRID_TEMPPATH
            );

            // Disable ADMIN window.
            EnableWindow(hwnd, FALSE);

            break;
        }

        case CHPASS_CREATE:
        {
            HWND hwndChpass = createChpassDialog(hwnd, hInstance);

            if (hwndChpass)
            {
                // Disable user window.
                EnableWindow(hwnd, FALSE);
            }

            // Send temp file path to password change window.
            sendData(
                hwndChpass,
                hwnd,
                pbTempPath,
                dwTempPathSize,
                SRID_TEMPPATH
            );

            // Store ADMIN name in BYTE array.
            DWORD dwUsernameSize = (wcslen(ADMIN_NAME) + 1) * sizeof(wchar_t);
            BYTE* pbUsername = new BYTE[dwUsernameSize];
            memcpy_s(pbUsername, dwUsernameSize, ADMIN_NAME, dwUsernameSize);

            // Send username to password change window.
            sendData(
                hwndChpass,
                hwnd,
                pbUsername,
                dwUsernameSize,
                SRID_USERNAME
            );

            break;
        }

        // Show information about program.
        case MENU_ABOUT:
            MessageBox(
                hwnd,
                wcAboutProgram,
                L"About program",
                MB_OK | MB_ICONINFORMATION
            );
            break;

        case USER_CLOSE:
            DestroyWindow(hwnd);
            break;

        case ADMIN_LIST:
            switch (HIWORD(wParam))
            {
            case LBN_SELCHANGE:
            {
                // Convert BYTE array to wstring for processing.
                std::wstring wstrTempPath(
                    (const wchar_t*)pbTempPath,
                    dwTempPathSize / sizeof(wchar_t)
                );

                // Show information about selected user.
                // Error handling is not necessary because either way we break.
                // The callee function is responsible for notifying about errors.
                showSelectedUserInfo(hwnd, wstrTempPath);
                break;
            }
            }
            break;
        }

        return 0;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

LRESULT CALLBACK UserProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Get current instance handle for window creation.
    static HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(NULL);

    if (!hInstance)
    {
        showError(L"UserProc::GetModuleHandle");
        return -1;
    }

    // Create buffer for temp file path.
    static BYTE* pbTempPath;
    static DWORD dwTempPathSize;

    // Create buffer for username.
    static BYTE* pbUsername;
    static DWORD dwUsernameSize;

    switch (uMsg)
    {
    case WM_CREATE:
        if (addMenu(hwnd) < 0 || addUserControls(hwnd) < 0)
        {
            DestroyWindow(hwnd);
        }
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

        EndPaint(hwnd, &ps);

        return 0;
    }

    case WM_DESTROY:
    {
        // Convert BYTE array to wstring for processing.
        std::wstring wstrTempPath(
            (const wchar_t*)pbTempPath,
            dwTempPathSize / sizeof(wchar_t)
        );

        // Encrypt temp file.
        if (encryptCreds(wstrTempPath) < 0)
        {
            showError(L"UserProc::encryptCreds");
        }

        // Cleanup.
        if (pbTempPath)
        {
            delete[] pbTempPath;
        }
        if (pbUsername)
        {
            delete[] pbUsername;
        }

        PostQuitMessage(0);
        return 0;
    }

    // Receive data from owner window.
    case WM_COPYDATA:
    {
        COPYDATASTRUCT* pcds = (COPYDATASTRUCT*)lParam;

        switch (pcds->dwData)
        {
        case SRID_TEMPPATH:
            // Get buffer size.
            dwTempPathSize = pcds->cbData;

            // Allocate memory for temp file path.
            pbTempPath = new BYTE[dwTempPathSize];

            // Write data.
            memcpy_s(pbTempPath, dwTempPathSize, pcds->lpData, pcds->cbData);

            break;

        case SRID_USERNAME:
            dwUsernameSize = pcds->cbData;

            pbUsername = new BYTE[dwUsernameSize];

            memcpy_s(pbUsername, dwUsernameSize, pcds->lpData, pcds->cbData);

            break;
        }

        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case CHPASS_CREATE:
        {
            HWND hwndChpass = createChpassDialog(hwnd, hInstance);

            if (hwndChpass)
            {
                // Disable user window.
                EnableWindow(hwnd, FALSE);
            }

            // Send temp file path to password change window.
            sendData(
                hwndChpass,
                hwnd,
                pbTempPath,
                dwTempPathSize,
                SRID_TEMPPATH
            );

            // Send username to password change window.
            sendData(
                hwndChpass,
                hwnd,
                pbUsername,
                dwUsernameSize,
                SRID_USERNAME
            );

            break;
        }

        // Show information about program.
        case MENU_ABOUT:
            MessageBox(
                hwnd,
                wcAboutProgram,
                L"About program",
                MB_OK | MB_ICONINFORMATION
            );
            break;

        case USER_CLOSE:
            DestroyWindow(hwnd);
            break;
        }

        return 0;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

LRESULT CALLBACK ChpassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Get owner window handle.
    static HWND hwndOwner = GetWindow(hwnd, GW_OWNER);

    // Create buffer for temp file path.
    static BYTE* pbTempPath;
    static DWORD dwTempPathSize;

    // Create buffer for username.
    static BYTE* pbUsername;
    static DWORD dwUsernameSize;

    switch (uMsg)
    {
    case WM_CREATE:
        if (addChpassControls(hwnd) < 0)
        {
            EnableWindow(hwndOwner, TRUE);
            DestroyWindow(hwnd);
        }
        return 0;

    case WM_CLOSE:
        // Cleanup.
        if (pbTempPath)
        {
            delete[] pbTempPath;
        }
        if (pbUsername)
        {
            delete[] pbUsername;
        }

        // Enable owner window before dialog is closed.
        EnableWindow(hwndOwner, TRUE);
        DestroyWindow(hwnd);
        return 0;

    // Receive data from owner window.
    case WM_COPYDATA:
    {
        COPYDATASTRUCT* pcds = (COPYDATASTRUCT*)lParam;

        switch (pcds->dwData)
        {
        case SRID_TEMPPATH:
            // Get buffer size.
            dwTempPathSize = pcds->cbData;

            // Allocate memory for temp file path.
            pbTempPath = new BYTE[dwTempPathSize];

            // Write data.
            memcpy_s(pbTempPath, dwTempPathSize, pcds->lpData, pcds->cbData);

            break;

        case SRID_USERNAME:
            dwUsernameSize = pcds->cbData;

            pbUsername = new BYTE[dwUsernameSize];

            memcpy_s(pbUsername, dwUsernameSize, pcds->lpData, pcds->cbData);

            break;
        }

        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case CHPASS_CHANGE:
        {
            // Convert BYTE arrays to wstrings for processing.
            std::wstring wstrTempPath(
                (const wchar_t*)pbTempPath,
                dwTempPathSize / sizeof(wchar_t)
            );
            std::wstring wstrUsername(
                (const wchar_t*)pbUsername,
                dwUsernameSize / sizeof(wchar_t)
            );

            if (changePassword(hwnd, hwndOwner, wstrUsername, wstrTempPath) < 0)
            {
                break;
            }

            // Enable owner window before dialog is closed.
            EnableWindow(hwndOwner, TRUE);
            DestroyWindow(hwnd);

            break;
        }

        case CHPASS_CANCEL:
            EnableWindow(hwndOwner, TRUE);
            DestroyWindow(hwnd);
            break;
            
        case CHPASS_CLOSE:
            EnableWindow(hwndOwner, TRUE);
            DestroyWindow(hwnd);
            DestroyWindow(hwndOwner);
            PostQuitMessage(0);
            break;
        }

        return 0;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

LRESULT CALLBACK AddUserProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Get owner window handle.
    static HWND hwndOwner = GetWindow(hwnd, GW_OWNER);

    // Create buffers for temp file path.
    static BYTE* pbTempPath;
    static DWORD dwTempPathSize;

    switch (uMsg)
    {
    case WM_CREATE:
        if (addAddUserControls(hwnd) < 0)
        {
            // Enable owner window before dialog is closed.
            EnableWindow(hwndOwner, TRUE);
            DestroyWindow(hwnd);
        }
        return 0;

    case WM_CLOSE:
        // Cleanup.
        if (pbTempPath)
        {
            delete[] pbTempPath;
        }

        EnableWindow(hwndOwner, TRUE);
        DestroyWindow(hwnd);
        return 0;

    // Receive data from owner window.
    case WM_COPYDATA:
    {
        COPYDATASTRUCT* pcds = (COPYDATASTRUCT*)lParam;

        switch (pcds->dwData)
        {
        case SRID_TEMPPATH:
            // Get buffer size.
            dwTempPathSize = pcds->cbData;

            // Allocate memory for temp file path.
            pbTempPath = new BYTE[dwTempPathSize];

            // Write data.
            memcpy_s(pbTempPath, dwTempPathSize, pcds->lpData, pcds->cbData);

            break;
        }

        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ADD_ADD:
        {
            // Convert BYTE array to wstring for processing.
            std::wstring wstrTempPath(
                (const wchar_t*)pbTempPath,
                dwTempPathSize / sizeof(wchar_t)
            );

            if (addUser(hwnd, hwndOwner, wstrTempPath) < 0)
            {
                break;
            }

            // Enable owner window before dialog is closed.
            EnableWindow(hwndOwner, TRUE);
            DestroyWindow(hwnd);

            break;
        }
            
        case ADD_CANCEL:
            EnableWindow(hwndOwner, TRUE);
            DestroyWindow(hwnd);
            break;
        }

        return 0;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

LRESULT CALLBACK EditUserProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Get owner window handle.
    static HWND hwndOwner = GetWindow(hwnd, GW_OWNER);
    
    // Create buffers for temp file path.
    static BYTE* pbTempPath;
    static DWORD dwTempPathSize;

    switch (uMsg)
    {
    case WM_CREATE:
        if (addEditUserControls(hwnd) < 0)
        {
            // Enable owner window before dialog is closed.
            EnableWindow(hwndOwner, TRUE);
            DestroyWindow(hwnd);
        }
        return 0;

    case WM_CLOSE:
        // Cleanup.
        if (pbTempPath)
        {
            delete[] pbTempPath;
        }

        EnableWindow(hwndOwner, TRUE);
        DestroyWindow(hwnd);
        return 0;

    // Receive data from owner window.
    case WM_COPYDATA:
    {
        COPYDATASTRUCT* pcds = (COPYDATASTRUCT*)lParam;

        switch (pcds->dwData)
        {
        case SRID_TEMPPATH:
            // Get buffer size.
            dwTempPathSize = pcds->cbData;

            // Allocate memory for temp file path.
            pbTempPath = new BYTE[dwTempPathSize];

            // Write data.
            memcpy_s(pbTempPath, dwTempPathSize, pcds->lpData, pcds->cbData);

            break;
        }

        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case EDIT_SET:
        {
            // Convert BYTE array to wstring for processing.
            std::wstring wstrTempPath(
                (const wchar_t*)pbTempPath,
                dwTempPathSize / sizeof(wchar_t)
            );

            if (editUser(hwnd, hwndOwner, wstrTempPath) < 0)
            {
                break;
            }

            // Enable owner window before dialog is closed.
            EnableWindow(hwndOwner, TRUE);
            DestroyWindow(hwnd);

            break;
        }

        case EDIT_CANCEL:
            EnableWindow(hwndOwner, TRUE);
            DestroyWindow(hwnd);
            break;
        }

        return 0;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

int addLoginControls(HWND hwndLogin)
{
    if (!CreateWindow(
        L"STATIC", L"Username:",
        WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET,
        TEXT_WIDTH - ELEMENT_OFFSET * 2,
        ELEMENT_HEIGHT,
        hwndLogin, NULL, NULL, NULL
    ))
    {
        showError(L"addLoginControls::STATIC::USERNAME");
        return -1;
    }

    if (!CreateWindow(
        L"EDIT", L"",
        ES_AUTOHSCROLL | WS_BORDER | WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET * 3,
        TEXT_WIDTH - ELEMENT_OFFSET * 2,
        ELEMENT_HEIGHT,
        hwndLogin, (HMENU)LOGIN_USERNAME, NULL, NULL
    ))
    {
        showError(L"addLoginControls::EDIT::USERNAME");
        return -1;
    }

    if (!CreateWindow(
        L"STATIC", L"Password:",
        WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET * 5,
        TEXT_WIDTH - ELEMENT_OFFSET * 2,
        ELEMENT_HEIGHT,
        hwndLogin, NULL, NULL, NULL
    ))
    {
        showError(L"addLoginControls::STATIC::PASSWORD");
        return -1;
    }

    if (!CreateWindow(
        L"EDIT", L"",
        ES_AUTOHSCROLL | ES_PASSWORD | WS_BORDER | WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET * 7,
        TEXT_WIDTH - ELEMENT_OFFSET * 2,
        ELEMENT_HEIGHT,
        hwndLogin, (HMENU)LOGIN_PASSWORD, NULL, NULL
    ))
    {
        showError(L"addLoginControls::EDIT::PASSWORD");
        return -1;
    }

    if (!CreateWindow(
        L"BUTTON", L"Login",
        WS_BORDER | WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        LOGIN_HEIGHT - ELEMENT_OFFSET * 7,
        BUTTON_WIDTH,
        ELEMENT_HEIGHT,
        hwndLogin, (HMENU)LOGIN, NULL, NULL
    ))
    {
        showError(L"addLoginControls::BUTTON::LOGIN");
        return -1;
    }

    if (!CreateWindow(
        L"BUTTON", L"Cancel",
        WS_BORDER | WS_CHILD | WS_VISIBLE,
        LOGIN_WIDTH - BUTTON_WIDTH - ELEMENT_OFFSET * 3,
        LOGIN_HEIGHT - ELEMENT_OFFSET * 7,
        BUTTON_WIDTH,
        ELEMENT_HEIGHT,
        hwndLogin, (HMENU)LOGIN_CLOSE, NULL, NULL
    ))
    {
        showError(L"addLoginControls::BUTTON::CANCEL");
        return -1;
    }

    return 0;
}

int addAdminControls(HWND hwndAdmin)
{
    if (!CreateWindow(
        L"STATIC", L"Registered users",
        WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET,
        (ADMIN_WIDTH) / 2 - ELEMENT_OFFSET * 2,
        ELEMENT_HEIGHT,
        hwndAdmin, NULL, NULL, NULL
    ))
    {
        showError(L"addAdminControls::STATIC::REGISTERED");
        return -1;
    }

    if (!CreateWindow(
        L"STATIC", L"User information",
        WS_CHILD | WS_VISIBLE,
        (ADMIN_WIDTH) / 2,
        ELEMENT_OFFSET,
        (ADMIN_WIDTH) / 2 - ELEMENT_OFFSET * 3,
        ELEMENT_HEIGHT,
        hwndAdmin, NULL, NULL, NULL
    ))
    {
        showError(L"addAdminControls::STATIC::INFORMATION");
        return -1;
    }

    if (!CreateWindow(
        L"LISTBOX", L"",
        LBS_STANDARD | WS_BORDER | WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET * 3,
        (ADMIN_WIDTH) / 2 - ELEMENT_OFFSET * 2,
        ELEMENT_HEIGHT * 6,
        hwndAdmin, (HMENU)ADMIN_LIST, NULL, NULL
    ))
    {
        showError(L"addAdminControls::LISTBOX");
        return -1;
    }

    if (!CreateWindow(
        L"STATIC", L"Name:",
        WS_CHILD | WS_VISIBLE,
        (ADMIN_WIDTH) / 2,
        ELEMENT_OFFSET * 4,
        (ADMIN_WIDTH) / 2 - ELEMENT_OFFSET * 3,
        ELEMENT_HEIGHT,
        hwndAdmin, NULL, NULL, NULL
    ))
    {
        showError(L"addAdminControls::STATIC::NAME");
        return -1;
    }

    if (!CreateWindow(
        L"STATIC", L"",
        WS_CHILD | WS_VISIBLE,
        (ADMIN_WIDTH) * 2 / 3,
        ELEMENT_OFFSET * 4,
        (ADMIN_WIDTH) / 4 - ELEMENT_OFFSET,
        ELEMENT_HEIGHT,
        hwndAdmin, (HMENU)ADMIN_USERNAME, NULL, NULL
    ))
    {
        showError(L"addAdminControls::STATIC::USERNAME");
        return -1;
    }

    if (!CreateWindow(
        L"BUTTON", L"",
        BS_CHECKBOX | WS_CHILD | WS_VISIBLE,
        (ADMIN_WIDTH) / 2,
        ELEMENT_OFFSET * 6,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET,
        hwndAdmin, (HMENU)ADMIN_PERMITTED, NULL, NULL
    ))
    {
        showError(L"addAdminControls::BUTTON::PERMITTED");
        return -1;
    }

    if (!CreateWindow(
        L"BUTTON", L"",
        BS_CHECKBOX | WS_CHILD | WS_VISIBLE,
        (ADMIN_WIDTH) / 2,
        ELEMENT_OFFSET * 8,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET,
        hwndAdmin, (HMENU)ADMIN_PWPOLICY, NULL, NULL
    ))
    {
        showError(L"addAdminControls::BUTTON::PWPOLICY");
        return -1;
    }

    if (!CreateWindow(
        L"STATIC", L"Permitted",
        WS_CHILD | WS_VISIBLE,
        (ADMIN_WIDTH) / 2 + ELEMENT_OFFSET * 2,
        ELEMENT_OFFSET * 6,
        (ADMIN_WIDTH) / 2 - ELEMENT_OFFSET * 5,
        ELEMENT_HEIGHT,
        hwndAdmin, NULL, NULL, NULL
    ))
    {
        showError(L"addAdminControls::STATIC::PERMITTED");
        return -1;
    }

    if (!CreateWindow(
        L"STATIC", L"Password policy",
        WS_CHILD | WS_VISIBLE,
        (ADMIN_WIDTH) / 2 + ELEMENT_OFFSET * 2,
        ELEMENT_OFFSET * 8,
        (ADMIN_WIDTH) / 2 - ELEMENT_OFFSET * 5,
        ELEMENT_HEIGHT,
        hwndAdmin, NULL, NULL, NULL
    ))
    {
        showError(L"addAdminControls::STATIC::PWPOLICY");
        return -1;
    }

    if (!CreateWindow(
        L"BUTTON", L"Add",
        WS_BORDER | WS_CHILD | WS_VISIBLE,
        (ADMIN_WIDTH) / 2,
        ELEMENT_OFFSET * 11,
        BUTTON_WIDTH,
        ELEMENT_HEIGHT,
        hwndAdmin, (HMENU)ADMIN_ADD, NULL, NULL
    ))
    {
        showError(L"addAdminControls::BUTTON::ADD");
        return -1;
    }

    if (!CreateWindow(
        L"BUTTON", L"Edit",
        WS_BORDER | WS_CHILD | WS_VISIBLE,
        (ADMIN_WIDTH) / 2 + BUTTON_WIDTH + ELEMENT_OFFSET,
        ELEMENT_OFFSET * 11,
        BUTTON_WIDTH,
        ELEMENT_HEIGHT,
        hwndAdmin, (HMENU)ADMIN_EDIT, NULL, NULL
    ))
    {
        showError(L"addAdminControls::BUTTON::EDIT");
        return -1;
    }

    return 0;
}

int addUserControls(HWND hwndUser)
{
    if (!CreateWindow(
        L"BUTTON", L"Change password",
        WS_BORDER | WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET,
        TEXT_WIDTH - ELEMENT_OFFSET,
        ELEMENT_HEIGHT,
        hwndUser, (HMENU)CHPASS_CREATE, NULL, NULL
    ))
    {
        showError(L"addUserControls::BUTTON::CHPASS");
        return -1;
    }

    if (!CreateWindow(
        L"BUTTON", L"Close",
        WS_BORDER | WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET * 4,
        TEXT_WIDTH - ELEMENT_OFFSET,
        ELEMENT_HEIGHT,
        hwndUser, (HMENU)USER_CLOSE, NULL, NULL
    ))
    {
        showError(L"addUserControls::BUTTON::CLOSE");
        return -1;
    }

    return 0;
}

int addChpassControls(HWND hwndChpass)
{
    if (!CreateWindow(
        L"STATIC", L"Current password:",
        WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET,
        TEXT_WIDTH - ELEMENT_OFFSET * 2,
        ELEMENT_HEIGHT,
        hwndChpass, NULL, NULL, NULL
    ))
    {
        showError(L"addChpassControls::STATIC::CURRENT");
        return -1;
    }

    if (!CreateWindow(
        L"EDIT", L"",
        ES_AUTOHSCROLL | ES_PASSWORD | WS_BORDER | WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET * 3,
        TEXT_WIDTH - ELEMENT_OFFSET * 2,
        ELEMENT_HEIGHT,
        hwndChpass, (HMENU)CHPASS_CURRENT, NULL, NULL
    ))
    {
        showError(L"addChpassControls::EDIT::CURRENT");
        return -1;
    }

    if (!CreateWindow(
        L"STATIC", L"New password:",
        WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET * 5,
        TEXT_WIDTH - ELEMENT_OFFSET * 2,
        ELEMENT_HEIGHT,
        hwndChpass, NULL, NULL, NULL
    ))
    {
        showError(L"addChpassControls::STATIC::NEW");
        return -1;
    }

    if (!CreateWindow(
        L"EDIT", L"",
        ES_AUTOHSCROLL | ES_PASSWORD | WS_BORDER | WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET * 7,
        TEXT_WIDTH - ELEMENT_OFFSET * 2,
        ELEMENT_HEIGHT,
        hwndChpass, (HMENU)CHPASS_NEW, NULL, NULL
    ))
    {
        showError(L"addChpassControls::EDIT::NEW");
        return -1;
    }

    if (!CreateWindow(
        L"STATIC", L"Confirm password:",
        WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET * 9,
        TEXT_WIDTH - ELEMENT_OFFSET * 2,
        ELEMENT_HEIGHT,
        hwndChpass, NULL, NULL, NULL
    ))
    {
        showError(L"addChpassControls::STATIC::CONFIRM");
        return -1;
    }

    if (!CreateWindow(
        L"EDIT", L"",
        ES_AUTOHSCROLL | ES_PASSWORD | WS_BORDER | WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET * 11,
        TEXT_WIDTH - ELEMENT_OFFSET * 2,
        ELEMENT_HEIGHT,
        hwndChpass, (HMENU)CHPASS_CONFIRM, NULL, NULL
    ))
    {
        showError(L"addChpassControls::EDIT::CONFIRM");
        return -1;
    }

    if (!CreateWindow(
        L"BUTTON", L"Change",
        WS_BORDER | WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        CHPASS_HEIGHT - ELEMENT_OFFSET * 10,
        BUTTON_WIDTH,
        ELEMENT_HEIGHT,
        hwndChpass, (HMENU)CHPASS_CHANGE, NULL, NULL
    ))
    {
        showError(L"addChpassControls::BUTTON::CHANGE");
        return -1;
    }

    if (!CreateWindow(
        L"BUTTON", L"Cancel",
        WS_BORDER | WS_CHILD | WS_VISIBLE,
        CHPASS_WIDTH - BUTTON_WIDTH - ELEMENT_OFFSET * 3,
        CHPASS_HEIGHT - ELEMENT_OFFSET * 10,
        BUTTON_WIDTH,
        ELEMENT_HEIGHT,
        hwndChpass, (HMENU)CHPASS_CANCEL, NULL, NULL
    ))
    {
        showError(L"addChpassControls::BUTTON::CANCEL");
        return -1;
    }

    if (!CreateWindow(
        L"BUTTON", L"Close",
        WS_BORDER | WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        CHPASS_HEIGHT - ELEMENT_OFFSET * 7,
        TEXT_WIDTH - ELEMENT_OFFSET * 2,
        ELEMENT_HEIGHT,
        hwndChpass, (HMENU)CHPASS_CLOSE, NULL, NULL
    ))
    {
        showError(L"addChpassControls::BUTTON::CLOSE");
        return -1;
    }

    return 0;
}

int addAddUserControls(HWND hwndAddUser)
{
    if (!CreateWindow(
        L"STATIC", L"Username:",
        WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET,
        TEXT_WIDTH - ELEMENT_OFFSET * 2,
        ELEMENT_HEIGHT,
        hwndAddUser, NULL, NULL, NULL
    ))
    {
        showError(L"addAddUserControls::STATIC::USERNAME");
        return -1;
    }

    if (!CreateWindow(
        L"EDIT", L"",
        ES_AUTOHSCROLL | WS_BORDER | WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET * 3,
        TEXT_WIDTH - ELEMENT_OFFSET * 2,
        ELEMENT_HEIGHT,
        hwndAddUser, (HMENU)ADD_USERNAME, NULL, NULL
    ))
    {
        showError(L"addAddUserControls::EDIT::USERNAME");
        return -1;
    }

    if (!CreateWindow(
        L"STATIC", L"Permitted",
        WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET * 3,
        ELEMENT_OFFSET * 6,
        (ADD_WIDTH) / 2 + ELEMENT_OFFSET * 5 / 2,
        ELEMENT_HEIGHT,
        hwndAddUser, NULL, NULL, NULL
    ))
    {
        showError(L"addAddUserControls::STATIC::PERMITTED");
        return -1;
    }

    if (!CreateWindow(
        L"BUTTON", L"",
        BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET * 6,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET,
        hwndAddUser, (HMENU)ADD_PERMITTED, NULL, NULL
    ))
    {
        showError(L"addAddUserControls::BUTTON::PERMITTED");
        return -1;
    }

    if (!CreateWindow(
        L"STATIC", L"Password policy",
        WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET * 3,
        ELEMENT_OFFSET * 8,
        (ADD_WIDTH) / 2 + ELEMENT_OFFSET * 5 / 2,
        ELEMENT_HEIGHT,
        hwndAddUser, NULL, NULL, NULL
    ))
    {
        showError(L"addAddUserControls::STATIC::PWPOLICY");
        return -1;
    }

    if (!CreateWindow(
        L"BUTTON", L"",
        BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET * 8,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET,
        hwndAddUser, (HMENU)ADD_PWPOLICY, NULL, NULL
    ))
    {
        showError(L"addAddUserControls::BUTTON::PWPOLICY");
        return -1;
    }

    if (!CreateWindow(
        L"BUTTON", L"Add",
        WS_BORDER | WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        ADD_HEIGHT - ELEMENT_OFFSET * 7,
        BUTTON_WIDTH,
        ELEMENT_HEIGHT,
        hwndAddUser, (HMENU)ADD_ADD, NULL, NULL
    ))
    {
        showError(L"addAddUserControls::BUTTON::ADD");
        return -1;
    }

    if (!CreateWindow(
        L"BUTTON", L"Cancel",
        WS_BORDER | WS_CHILD | WS_VISIBLE,
        (ADD_WIDTH) / 2,
        ADD_HEIGHT - ELEMENT_OFFSET * 7,
        BUTTON_WIDTH,
        ELEMENT_HEIGHT,
        hwndAddUser, (HMENU)ADD_CANCEL, NULL, NULL
    ))
    {
        showError(L"addAddUserControls::BUTTON::PWPOLICY");
        return -1;
    }

    return 0;
}

int addEditUserControls(HWND hwndEditUser)
{
    if (!CreateWindow(
        L"STATIC", L"Permitted",
        WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET * 3,
        ELEMENT_OFFSET,
        (EDIT_WIDTH) / 2 + ELEMENT_OFFSET * 5 / 2,
        ELEMENT_HEIGHT,
        hwndEditUser, NULL, NULL, NULL
    ))
    {
        showError(L"addEditUserControls::STATIC::PERMITTED");
        return -1;
    }

    if (!CreateWindow(
        L"BUTTON", L"",
        BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET,
        hwndEditUser, (HMENU)EDIT_PERMITTED, NULL, NULL
    ))
    {
        showError(L"addEditUserControls::BUTTON::PERMITTED");
        return -1;
    }

    if (!CreateWindow(
        L"STATIC", L"Password policy",
        WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET * 3,
        ELEMENT_OFFSET * 3,
        (EDIT_WIDTH) / 2 + ELEMENT_OFFSET * 5 / 2,
        ELEMENT_HEIGHT,
        hwndEditUser, NULL, NULL, NULL
    ))
    {
        showError(L"addEditUserControls::STATIC::PWPOLICY");
        return -1;
    }

    if (!CreateWindow(
        L"BUTTON", L"",
        BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET * 3,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET,
        hwndEditUser, (HMENU)EDIT_PWPOLICY, NULL, NULL
    ))
    {
        showError(L"addEditUserControls::BUTTON::PWPOLICY");
        return -1;
    }

    if (!CreateWindow(
        L"BUTTON", L"Set",
        WS_BORDER | WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        EDIT_HEIGHT - ELEMENT_OFFSET * 7,
        BUTTON_WIDTH,
        ELEMENT_HEIGHT,
        hwndEditUser, (HMENU)EDIT_SET, NULL, NULL
    ))
    {
        showError(L"addEditUserControls::BUTTON::SET");
        return -1;
    }

    if (!CreateWindow(
        L"BUTTON", L"Cancel",
        WS_BORDER | WS_CHILD | WS_VISIBLE,
        (EDIT_WIDTH) / 2,
        EDIT_HEIGHT - ELEMENT_OFFSET * 7,
        BUTTON_WIDTH,
        ELEMENT_HEIGHT,
        hwndEditUser, (HMENU)EDIT_CANCEL, NULL, NULL
    ))
    {
        showError(L"addEditUserControls::BUTTON::CANCEL");
        return -1;
    }

    return 0;
}

int addMenu(HWND hwnd)
{
    // Create and add menu bar.
    HMENU hMenuBar = CreateMenu();
    HMENU hMenuFile = CreateMenu();
    HMENU hMenuHelp = CreateMenu();

    if (!hMenuBar || !hMenuFile || !hMenuHelp)
    {
        showError(L"addMenu::CreateMenu");
        return -1;
    }

    // Create menu options.
    if (!AppendMenu(hMenuHelp, MF_STRING, MENU_ABOUT, L"About program"))
    {
        showError(L"addMenu::ABOUT::AppendMenu");
        return -1;
    }
    if (!AppendMenu(hMenuFile, MF_STRING, CHPASS_CREATE, L"Change password"))
    {
        showError(L"addMenu::CHPASS::AppendMenu");
        return -1;
    }
    if (!AppendMenu(hMenuFile, MF_SEPARATOR, 0, NULL))
    {
        showError(L"addMenu::SEPARATOR::AppendMenu");
        return -1;
    }
    if (!AppendMenu(hMenuFile, MF_STRING, USER_CLOSE, L"Exit"))
    {
        showError(L"addMenu::EXIT::AppendMenu");
        return -1;
    }

    // Add created options to menu bar.
    if (!AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hMenuFile, L"File"))
    {
        showError(L"addMenu::FILE::AppendMenu");
        return -1;
    }
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

HWND createChpassDialog(HWND hwndOwner, HINSTANCE hInstance)
{
    HWND hwnd = CreateWindow(
        CHPASS_CLASS,
        L"Password change",
        WS_OVERLAPPED | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CHPASS_WIDTH, CHPASS_HEIGHT,
        hwndOwner, NULL, hInstance, NULL
    );

    if (hwnd)
    {
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
    }
    else
    {
        showError(L"createChpassDialog::CreateWindow");
    }

    return hwnd;
}

int createListBox(HWND hwndAdmin, const std::wstring& wstrPath)
{
    // Read all credentials.
    std::string strAllCreds;

    if (readCreds(strAllCreds, wstrPath) < 0)
    {
        return -1;
    }

    // Convert data.
    std::wstring wstrAllCreds;

    if (toWide(wstrAllCreds, strAllCreds) < 0)
    {
        return -1;
    }

    // Get handle to user list.
    HWND hwndList = GetDlgItem(hwndAdmin, ADMIN_LIST);

    if (hwndList == NULL)
    {
        showError(L"createListBox::GetDlgItem");
        return -1;
    }

    // Parse credentials.
    int iEnd;

    while (!wstrAllCreds.empty())
    {
        // Extract entry.
        iEnd = wstrAllCreds.find(L"\n");
        if (iEnd == std::wstring::npos)
        {
            break;
        }

        // Parse extracted entry.
        User stTemp;

        if (parseEntry(&stTemp, wstrAllCreds) < 0)
        {
            break;
        }

        // Add user to list box.
        SendMessage(
            hwndList,
            LB_ADDSTRING,
            0,
            (LPARAM)stTemp.wstrUsername.c_str()
        );

        wstrAllCreds = wstrAllCreds.substr(iEnd + 1);
    }

    // Set input focus to the list box.
    SetFocus(hwndList);

    return 0;
}

int showSelectedUserInfo(HWND hwndAdmin, const std::wstring& wstrPath)
{
    // Get handle to user list.
    HWND hwndList = GetDlgItem(hwndAdmin, ADMIN_LIST);

    if (hwndList == NULL)
    {
        showError(L"showSelectedUserInfo::LIST::GetDlgItem");
        return -1;
    }

    // Get selected index.
    // First list item has index 0.
    DWORD dwItem = SendMessage(hwndList, LB_GETCURSEL, 0, 0);

    if (dwItem == LB_ERR)
    {
        showError(L"showSelectedUserInfo::LIST::GETCURSEL::SendMessage");
        return -1;
    }

    User stUser;

    // Resize username buffer.
    stUser.wstrUsername.resize(USERNAME_SIZE + 1, 0);

    // Get username.
    int iSize = SendMessage(
        hwndList,
        LB_GETTEXT,
        dwItem,
        (LPARAM)stUser.wstrUsername.c_str()
    );

    if (iSize == LB_ERR)
    {
        showError(L"showSelectedUserInfo::LIST::GETTEXT::SendMessage");
        return -1;
    }

    // Resize buffer to remove extra null bytes.
    stUser.wstrUsername.resize(iSize);

    // Get user parameters.
    if (getCreds(&stUser, wstrPath) < 0)
    {
        return -1;
    }

    // Set username.
    // Get handle of username field.
    HWND hwndUsername = GetDlgItem(hwndAdmin, ADMIN_USERNAME);

    if (hwndUsername == NULL)
    {
        showError(L"showSelectedUserInfo::USERNAME::GetDlgItem");
        return -1;
    }

    // Set username.
    if (!SetWindowText(hwndUsername, stUser.wstrUsername.c_str()))
    {
        showError(L"showSelectedUserInfo::USERNAME::SetWindowText");
        return -1;
    }

    // Set checkboxes' states.
    {
        // Get handles of checkboxes.
        HWND hwndPermitted = GetDlgItem(hwndAdmin, ADMIN_PERMITTED);

        if (hwndPermitted == NULL)
        {
            showError(L"showSelectedUserInfo::PERMITTED::GetDlgItem");
            return -1;
        }

        HWND hwndPasswordPolicy = GetDlgItem(hwndAdmin, ADMIN_PWPOLICY);

        if (hwndPasswordPolicy == NULL)
        {
            showError(L"showSelectedUserInfo::PWPOLICY::GetDlgItem");
            return -1;
        }

        // Set new states to checkboxes.
        SendMessage(hwndPermitted, BM_SETCHECK, stUser.bStatus, 0);
        SendMessage(hwndPasswordPolicy, BM_SETCHECK, stUser.bPasswordPolicy, 0);
    }

    return 0;
}

void sendData(HWND hwndReceiver, HWND hwndSender, const BYTE* pbData, DWORD dwSize, DWORD dwID)
{
    // Create struct to contain data.
    COPYDATASTRUCT cds;
    cds.dwData = dwID;
    cds.cbData = dwSize;
    cds.lpData = (BYTE*)pbData;

    // Send data to target.
    SendMessage(
        hwndReceiver,
        WM_COPYDATA,
        (WPARAM)hwndSender,
        (LPARAM)(LPVOID)&cds
    );

    return;
}

int identify(std::wstring& wstrPath, HWND hwndLogin, CurrentUser* pCurrentUser)
{
    // Get and check username.
    {
        // Create buffer for username.
        std::wstring wstrUsername(USERNAME_SIZE + 1, 0);

        // Get handle to username field.
        HWND hwndUsername = GetDlgItem(hwndLogin, LOGIN_USERNAME);

        if (hwndUsername == NULL)
        {
            showError(L"identify::GetDlgItem");
            return -1;
        }

        // Get username.
        int iSize = GetWindowText(hwndUsername, &wstrUsername[0], USERNAME_SIZE);

        if (!iSize)
        {
            if (!GetLastError())
            {
                MessageBox(
                    NULL,
                    L"Username is empty",
                    L"Alert",
                    MB_OK | MB_ICONWARNING
                );
            }
            else
            {
                showError(L"identify::GetWindowText");
            }
            return -1;
        }

        // Resize buffer to remove extra null bytes.
        wstrUsername.resize(iSize);

        // Check if username contains banned characters.
        if (!isValid(wstrUsername, USER_NO_PWPOLICY))
        {
            MessageBox(
                NULL,
                L"Username contains forbidden characters",
                L"Alert",
                MB_OK | MB_ICONWARNING
            );
            return -1;
        }

        // Check if the same user tries to login multiple times.
        if (pCurrentUser->wstrUsername.compare(wstrUsername) != 0)
        {
            pCurrentUser->iTries = LOGIN_TRIES;
        }

        // Set new user.
        pCurrentUser->wstrUsername = wstrUsername;
    }

    // Decrypt credentials file to temp file.
    if (decryptCreds(wstrPath) < 0)
    {
        showError(L"identify::decryptCreds");
        return -1;
    }

    // Check if user is registered.
    {
        // Read all credentials.
        std::string strAllCreds;

        if (readCreds(strAllCreds, wstrPath) < 0)
        {
            return -1;
        }

        // Convert data.
        std::wstring wstrAllCreds;

        if (toWide(wstrAllCreds, strAllCreds) < 0)
        {
            return -1;
        }

        // Try to find user's entry.
        std::vector<int> vError = { -1, -1 };

        if (findEntry(pCurrentUser->wstrUsername, wstrAllCreds) == vError)
        {
            return 0;
        }
    }

    return 1;
}

int authenticate(const std::wstring& wstrPath, HWND hwndLogin, CurrentUser* pCurrentUser)
{
    // Get user password.
    std::wstring wstrPassword(PASSWORD_SIZE + 1, 0);

    {
        // Get handle to password field.
        HWND hwndPassword = GetDlgItem(hwndLogin, LOGIN_PASSWORD);

        if (hwndPassword == NULL)
        {
            showError(L"authenticate::GetDlgItem");
            return -1;
        }

        // Get password.
        int iSize = GetWindowText(hwndPassword, &wstrPassword[0], PASSWORD_SIZE);

        // Authentication fails only when input is empty AND GetWindowText fails.
        if (!iSize && GetLastError())
        {
            // Empty password may be passed in some cases:
            // 1. The first run of the program as ADMIN.
            // 2. User is not limited by password policy.
            showError(L"authenticate::GetWindowText");
            return -1;
        }

        // Resize buffer to remove extra null bytes.
        wstrPassword.resize(iSize);
    }

    // Get user credentials.
    User stUser;

    stUser.wstrUsername = pCurrentUser->wstrUsername;

    // Try to find user among registered users.
    // We do not error handle this time because
    // we do not want to give user any clue about registered users.
    int iRegistered = getCreds(&stUser, wstrPath);

    // Compare entered and registered passwords.
    if (stUser.wstrPassword.compare(wstrPassword) != 0
        || iRegistered < 0)
    {
        // Decrease number of available login tries.
        pCurrentUser->iTries--;

        // Check if user has any tries left.
        if (pCurrentUser->iTries != 0)
        {
            std::wstring wstrMessage = L"Incorrect password. Tries left: "
                + std::to_wstring(pCurrentUser->iTries);
            
            MessageBox(
                NULL,
                wstrMessage.c_str(),
                L"Alert",
                MB_OK | MB_ICONWARNING
            );
        }
        else
        {
            MessageBox(
                NULL,
                L"Incorrect password. No tries left.",
                L"Error",
                MB_OK | MB_ICONERROR
            );
            DestroyWindow(hwndLogin);
        }
        return -1;
    }

    // Check if user is not blocked.
    if (stUser.bStatus == USER_BLOCKED)
    {
        MessageBox(
            NULL,
            L"User is blocked",
            L"Alert",
            MB_OK | MB_ICONWARNING
        );
        return -1;
    }

    return 0;
}

HWND authorize(const std::wstring& wstrUsername)
{
    // Create user window.
    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(NULL);

    if (!hInstance)
    {
        showError(L"authorize::GetWindowLongW");
        return NULL;
    }

    // Choose window type depending on user privileges.
    HWND hwnd;

    if (wstrUsername.compare(ADMIN_NAME) == 0)
    {
        hwnd = CreateWindow(
            ADMIN_CLASS, ADMIN_NAME,
            WS_OVERLAPPED | WS_SYSMENU,
            CW_USEDEFAULT, CW_USEDEFAULT,
            ADMIN_WIDTH, ADMIN_HEIGHT,
            NULL, NULL, hInstance, NULL
        );
    }
    else
    {
        hwnd = CreateWindow(
            USER_CLASS, wstrUsername.c_str(),
            WS_OVERLAPPED | WS_SYSMENU,
            CW_USEDEFAULT, CW_USEDEFAULT,
            USER_WIDTH, USER_HEIGHT,
            NULL, NULL, hInstance, NULL
        );
    }

    if (hwnd)
    {
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
    }
    else
    {
        showError(L"authorize::CreateWindow");
    }

    return hwnd;
}

int changePassword(HWND hwndChpass, HWND hwndOwner, const std::wstring& wstrUsername, const std::wstring& wstrPath)
{
    // Obtain registered password.
    User stUser;

    stUser.wstrUsername = wstrUsername;

    if (getCreds(&stUser, wstrPath) < 0)
    {
        return -1;
    }
    
    // Check if entered password is correct.
    {
        // Get handle to current password field.
        HWND hwndCurrent = GetDlgItem(hwndChpass, CHPASS_CURRENT);

        if (hwndCurrent == NULL)
        {
            showError(L"changePassword::CURRENT::GetDlgItem");
            return -1;
        }

        // Get current password.
        std::wstring wstrCurrent(PASSWORD_SIZE + 1, 0);

        int iSize = GetWindowText(hwndCurrent, &wstrCurrent[0], PASSWORD_SIZE);

        // Authentication fails only when input is empty AND GetWindowText fails.
        if (!iSize && GetLastError())
        {
            // Empty password may be passed in some cases:
            // 1. The first run of the program as ADMIN.
            // 2. User is not limited by password policy.
            showError(L"changePassword::GetWindowText");
            return -1;
        }

        // Resize buffer to remove extra null bytes.
        wstrCurrent.resize(iSize);

        // Compare entered and registered passwords.
        if (stUser.wstrPassword.compare(wstrCurrent) != 0)
        {
            MessageBox(
                NULL,
                L"Incorrect current password",
                L"Alert",
                MB_OK | MB_ICONWARNING
            );
            return -1;
        }
    }

    // Get and check new password.
    // For productivity sake we immediately write new password into User struct.
    stUser.wstrPassword.resize(PASSWORD_SIZE + 1);

    {
        // Get handle to new password field.
        HWND hwndNew = GetDlgItem(hwndChpass, CHPASS_NEW);

        if (hwndNew == NULL)
        {
            showError(L"changePassword::NEW::GetDlgItem");
            return -1;
        }

        // Get new password.
        int iSize = GetWindowText(hwndNew, &stUser.wstrPassword[0], PASSWORD_SIZE);

        if (!iSize && GetLastError())
        {
            showError(L"changePassword::NEW::GetWindowText");
            return -1;
        }

        // Resize buffer to remove extra null bytes.
        stUser.wstrPassword.resize(iSize);

        // Check if new password does not violate password policy.
        if (!isValid(stUser.wstrPassword, stUser.bPasswordPolicy))
        {
            MessageBox(
                NULL,
                L"New password violates password policy",
                L"Alert",
                MB_OK | MB_ICONWARNING
            );
            return -1;
        }
    }

    // Get confirmation password.
    std::wstring wstrConfirm(PASSWORD_SIZE + 1, 0);

    {
        // Get handle to password confirmation field.
        HWND hwndConfirm = GetDlgItem(hwndChpass, CHPASS_CONFIRM);

        if (hwndConfirm == NULL)
        {
            showError(L"changePassword::CONFIRM::GetDlgItem");
            return -1;
        }

        // Get confirmation password.
        int iSize = GetWindowText(hwndConfirm, &wstrConfirm[0], PASSWORD_SIZE);

        if (!iSize && GetLastError())
        {
            showError(L"changePassword::CONFIRM::GetWindowText");
            return -1;
        }

        // Resize buffer to remove extra null bytes.
        wstrConfirm.resize(iSize);

        // We do not need to check if confirmation password contains banned characters.
        // It only must match new password.
    }

    // Check if entered new and confirmation passwords match.
    if (stUser.wstrPassword.compare(wstrConfirm) != 0)
    {
        MessageBox(
            NULL,
            L"New and confirmation passwords do not match",
            L"Alert",
            MB_OK | MB_ICONWARNING
        );
        return -1;
    }

    // Save changes.
    if (setCreds(&stUser, wstrPath) < 0)
    {
        return -1;
    }

    // Set focus to owner window.
    SetFocus(hwndOwner);

    return 0;
}

int addUser(HWND hwndAddUser, HWND hwndOwner, const std::wstring& wstrPath)
{
    // Get and check username.
    User stUser;

    {
        stUser.wstrUsername.resize(USERNAME_SIZE + 1);

        // Get handle to username field.
        HWND hwndUsername = GetDlgItem(hwndAddUser, ADD_USERNAME);

        if (hwndUsername == NULL)
        {
            showError(L"addUser::ADD::GetDlgItem");
            return -1;
        }

        // Get username.
        int iSize = GetWindowText(hwndUsername, &stUser.wstrUsername[0], USERNAME_SIZE);

        if (!iSize)
        {
            if (!GetLastError())
            {
                MessageBox(
                    NULL,
                    L"Username is empty",
                    L"Alert",
                    MB_OK | MB_ICONWARNING
                );
            }
            else
            {
                showError(L"addUser::USERNAME::GetWindowText");
            }
            return -1;
        }

        // Resize buffer to remove extra null bytes.
        stUser.wstrUsername.resize(iSize);

        // Check if username does not contain banned characters.
        if (!isValid(stUser.wstrUsername, USER_NO_PWPOLICY))
        {
            MessageBox(
                NULL,
                L"Username contains forbidden characters",
                L"Alert",
                MB_OK | MB_ICONWARNING
            );
            return -1;
        }
    }

    // Check if user is not registered.
    {
        // Read all credentials.
        std::string strAllCreds;

        if (readCreds(strAllCreds, wstrPath) < 0)
        {
            return -1;
        }

        // Convert data.
        std::wstring wstrAllCreds;

        if (toWide(wstrAllCreds, strAllCreds) < 0)
        {
            return -1;
        }

        // Try to find user's entry.
        std::vector<int> vError = { -1, -1 };

        if (findEntry(stUser.wstrUsername, wstrPath) != vError)
        {
            MessageBox(
                NULL,
                L"Specified user is registered",
                L"Alert",
                MB_OK | MB_ICONWARNING
            );
            return -1;
        }
    }

    // Get user parameters from checkboxes.
    {
        // Get handles to parameter checkboxes.
        HWND hwndPermitted = GetDlgItem(hwndAddUser, ADD_PERMITTED);

        if (hwndPermitted == NULL)
        {
            showError(L"addUser::PERMITTED::GetDlgItem");
            return -1;
        }

        HWND hwndPasswordPolicy = GetDlgItem(hwndAddUser, ADD_PWPOLICY);

        if (hwndPasswordPolicy == NULL)
        {
            showError(L"addUser::PWPOLICY::GetDlgItem");
            return -1;
        }

        // Get user parameters.
        stUser.bStatus = SendMessage(hwndPermitted, BM_GETCHECK, 0, 0);
        stUser.bPasswordPolicy = SendMessage(hwndPasswordPolicy, BM_GETCHECK, 0, 0);
    }

    // Set user parameters.
    if (setCreds(&stUser, wstrPath) < 0)
    {
        return -1;
    }

    // Update user list.
    {
        // Get handle to user list.
        HWND hwndList = GetDlgItem(hwndOwner, ADMIN_LIST);
        if (hwndList == NULL)
        {
            showError(L"addUser::LIST::GetDlgItem");
            return -1;
        }

        // Add user to list box.
        if (SendMessage(
            hwndList,
            LB_ADDSTRING,
            0,
            (LPARAM)stUser.wstrUsername.c_str()
        ) == LB_ERR)
        {
            showError(L"addUser::LIST::ADDSTRING::SendMessage");
            return -1;
        }
    }

    // Set focus to owner window.
    SetFocus(hwndOwner);

    return 0;
}

int editUser(HWND hwndEditUser, HWND hwndOwner, const std::wstring& wstrPath)
{
    // Get selected username.
    User stUser;

    {
        // Get handle to user list.
        HWND hwndList = GetDlgItem(hwndOwner, ADMIN_LIST);

        if (hwndList == NULL)
        {
            showError(L"editUser::LIST::GetDlgItem");
            return -1;
        }

        // Get selected index.
        DWORD dwItem = SendMessage(hwndList, LB_GETCURSEL, 0, 0);

        if (dwItem <= 0)
        {
            showError(L"editUser::LIST::SendMessage");
            return -1;
        }

        // Resize username buffer to fit data.
        stUser.wstrUsername.resize(USERNAME_SIZE + 1);

        // Get username.
        int iSize = SendMessage(
            hwndList,
            LB_GETTEXT,
            dwItem,
            (LPARAM)stUser.wstrUsername.c_str()
        );

        if (iSize == LB_ERR)
        {
            showError(L"editUser::LIST::GETTEXT::SendMessage");
            return -1;
        }

        // Resize buffer to remove extra null bytes.
        stUser.wstrUsername.resize(iSize);
    }

    // Get password.
    if (getCreds(&stUser, wstrPath) < 0)
    {
        return -1;
    }

    // Get user parameters from checkboxes.
    {
        // Get handles to parameter checkboxes.
        HWND hwndPermitted = GetDlgItem(hwndEditUser, EDIT_PERMITTED);

        if (hwndPermitted == NULL)
        {
            showError(L"editUser()::PERMITTED::GetDlgItem()");
            return -1;
        }

        HWND hwndPasswordPolicy = GetDlgItem(hwndEditUser, EDIT_PWPOLICY);

        if (hwndPasswordPolicy == NULL)
        {
            showError(L"editUser()::PWPOLICY::GetDlgItem()");
            return -1;
        }

        // Get user parameters.
        stUser.bStatus = SendMessage(hwndPermitted, BM_GETCHECK, 0, 0);
        stUser.bPasswordPolicy = SendMessage(hwndPasswordPolicy, BM_GETCHECK, 0, 0);
    }

    // Update user parameters.
    if (setCreds(&stUser, wstrPath) < 0)
    {
        return -1;
    }

    // Set focus to owner window.
    SetFocus(hwndOwner);

    return 0;
}

int createCreds()
{
    // Set default ADMIN credentials.
    User stUser = { ADMIN_NAME, L"", USER_PERMITTED, USER_PWPOLICY };

    // Create ADMIN entry string.
    std::wstring wstrEntry = createEntry(&stUser);

    // Convert data.
    std::string strEntry;

    if (toMultibyte(strEntry, wstrEntry) < 0)
    {
        return -1;
    }

    // Encrypt entry.
    // Obtain key from ADMIN passphrase.
    HCRYPTKEY hKey;
    HCRYPTPROV hProv;

    // Initialize cryptography provider.
    if (!CryptAcquireContext(
        &hProv,
        NULL,
        NULL,
        PROV_RSA_AES,
        CRYPT_VERIFYCONTEXT
    ))
    {
        showError(L"createCreds::CryptAcquireContext");
        return -1;
    }

    if (getKey(&hProv, &hKey) < 0)
    {
        CryptReleaseContext(hProv, 0);
        return -1;
    }
    
    // Encrypt ADMIN entry string.
    BYTE* pbEncryptedEntry = NULL;
    DWORD dwSize = encrypt(
        hKey,
        &pbEncryptedEntry,
        (BYTE*)strEntry.c_str(),
        strEntry.size() + 1
    );

    if (dwSize < 0)
    {
        CryptDestroyKey(hKey);
        CryptReleaseContext(hProv, 0);
        return -1;
    }

    // Write encrypted entry into credentials file.
    if (writeCreds(
        pbEncryptedEntry,
        dwSize,
        wcCredsFilePath,
        std::ios::out) < 0)
    {
        CryptDestroyKey(hKey);
        CryptReleaseContext(hProv, 0);
        delete[] pbEncryptedEntry;
        return -1;
    }

    // Cleanup.
    CryptDestroyKey(hKey);
    CryptReleaseContext(hProv, 0);
    delete[] pbEncryptedEntry;

    return 0;
}

int readCreds(std::string& strCreds, const std::wstring& wstrPath)
{
    // Open file for reading.
    std::ifstream credsFile(wstrPath, std::ios::binary);

    if (!credsFile.is_open())
    {
        showError(L"readCreds::std::ifstream::is_open");
        return -1;
    }

    // Read file contents
    strCreds.assign(
        (std::istreambuf_iterator<char>(credsFile)),
        (std::istreambuf_iterator<char>())
    );

    // Cleanup.
    credsFile.close();

    return 0;
}

int writeCreds(const BYTE* pbCreds, DWORD dwSize, const std::wstring& wstrPath, std::ios::openmode mode)
{
    // Open file for writing.
    std::ofstream credsFile(wstrPath, mode | std::ios::binary);

    if (!credsFile.is_open())
    {
        showError(L"setCreds::std::ofstream::open");
        return -1;
    }

    // Write entry to file.
    credsFile.write((const char*)pbCreds, dwSize);

    // Cleanup.
    credsFile.close();
}

int getCreds(User* pUser, const std::wstring& wstrPath)
{
    // Read all credentials.
    std::string strAllCreds;

    if (readCreds(strAllCreds, wstrPath) < 0)
    {
        return -1;
    }
    
    // Convert data.
    std::wstring wstrAllCreds;

    if (toWide(wstrAllCreds, strAllCreds) < 0)
    {
        return -1;
    }

    // Find position [0] and length [1] of entry.
    std::vector<int> vIndices = findEntry(pUser->wstrUsername, wstrAllCreds);

    if (vIndices[0] == -1 && vIndices[1] == -1)
    {
        return -1;
    }

    // Store entry.
    std::wstring wstrUserCreds;

    wstrUserCreds = wstrAllCreds.substr(vIndices[0], vIndices[1]); // do not include line feed

    // Parse entry and fill User structure.
    if (parseEntry(pUser, wstrUserCreds) < 0)
    {
        return -1;
    }

    return 0;
}

std::wstring createEntry(User* pUser)
{
    // Remove extra null bytes.
    pUser->wstrUsername.erase(
        std::find(pUser->wstrUsername.begin(), pUser->wstrUsername.end(), '\0'),
        pUser->wstrUsername.end()
    );

    pUser->wstrPassword.erase(
        std::find(pUser->wstrPassword.begin(), pUser->wstrPassword.end(), '\0'),
        pUser->wstrPassword.end()
    );

    std::wstring wstrEntry = pUser->wstrUsername + CREDS_DELIMITER
        + pUser->wstrPassword + CREDS_DELIMITER
        + std::to_wstring(pUser->bStatus) + CREDS_DELIMITER
        + std::to_wstring(pUser->bPasswordPolicy) + L"\n";

    return wstrEntry;
}

std::vector<int> findEntry(const std::wstring& wstrUsername, const std::wstring& wstrAllCreds)
{
    // We need to append delimiter to avoid deleting wrong user.
    // Example: trying to delete user "bob" we may accidently delete "bob123".
    std::wstring wstr = wstrUsername + CREDS_DELIMITER;

    // Get user credentials entry.
    int iEntryPosition = wstrAllCreds.find(wstr.c_str());

    if (iEntryPosition == std::wstring::npos)
    {
        return { -1, -1 };
    }

    // Store location of entry that is to be removed.
    int iEntryLength = 0;

    for (int i = iEntryPosition; i < wstrAllCreds.size() + 1; i++)
    {
        if (wstrAllCreds[i] == 0 || wstrAllCreds[i] == L'\n')
        {
            iEntryLength = i - iEntryPosition;
            break;
        }
    }

    return { iEntryPosition, iEntryLength };
}

int parseEntry(User* pUser, const std::wstring& wstrEntry)
{
    // Store data in struct.
    std::wstring wstr(wstrEntry.c_str());
    int iBeginIndex;
    int iDelimiterIndex;

    iBeginIndex = 0;
    if (!pUser->wstrUsername.empty())
    {
        // Skip username if it is provided in struct.
        iDelimiterIndex = wstr.find(CREDS_DELIMITER);
        wstr = wstr.substr(iDelimiterIndex + 1);
    }
    else
    {
        iDelimiterIndex = wstr.find(CREDS_DELIMITER);
        pUser->wstrUsername = wstr.substr(0, iDelimiterIndex);
        if (pUser->wstrUsername.empty())
        {
            showError(L"parseEntry()::USERNAME");
            return -1;
        }
        iBeginIndex = iDelimiterIndex + 1;
    }

    wstr = wstr.substr(iBeginIndex);
    iDelimiterIndex = wstr.find(CREDS_DELIMITER);
    pUser->wstrPassword = wstr.substr(0, iDelimiterIndex);
    if (pUser->wstrPassword.empty() && iDelimiterIndex != 0) // password may be empty
    {
        showError(L"parseEntry()::PASSWORD");
        return -1;
    }

    std::wstring wstrNumber;

    iBeginIndex = iDelimiterIndex + 1;
    wstr = wstr.substr(iBeginIndex);
    iDelimiterIndex = wstr.find(CREDS_DELIMITER);
    wstrNumber = wstr.substr(0, iDelimiterIndex);
    pUser->bStatus = stoi(wstrNumber);
    if (pUser->bStatus == USER_INVALIDVALUE)
    {
        showError(L"parseEntry()::STATUS");
        return -1;
    }

    iBeginIndex = iDelimiterIndex + 1;
    iDelimiterIndex = wstr.find(L"\n");
    wstrNumber = wstr.substr(iBeginIndex, iDelimiterIndex);
    pUser->bPasswordPolicy = stoi(wstrNumber);
    if (pUser->bPasswordPolicy == USER_INVALIDVALUE)
    {
        showError(L"parseEntry()::PWPOLICY");
        return -1;
    }

    return 0;
}

int setCreds(User* pUser, const std::wstring& wstrPath)
{
    // Check if username is passed.
    if (pUser->wstrUsername.empty())
    {
        return -1;
    }

    // Remove previous entry if it exists.
    {
        // Read all credentials.
        std::string strAllCreds;

        if (readCreds(strAllCreds, wstrPath) < 0)
        {
            return -1;
        }

        // Convert data.
        std::wstring wstrAllCreds;

        if (toWide(wstrAllCreds, strAllCreds) < 0)
        {
            return -1;
        }

        // Try to find user's entry.
        std::vector<int> vError = { -1, -1 };

        if (findEntry(pUser->wstrUsername, wstrAllCreds) != vError)
        {
            if (removeCreds(pUser->wstrUsername, wstrPath) < 0)
            {
                return -1;
            }
        }
    }

    // Create entry string.
    std::wstring wstrEntry = createEntry(pUser);

    // Convert entry string.
    std::string strEntry;

    if (toMultibyte(strEntry, wstrEntry) < 0)
    {
        return -1;
    }

    // Add new credentials to file.
    if (writeCreds(
        (BYTE*)strEntry.c_str(),
        strEntry.size() - 1, // do not include null byte.
        wstrPath,
        std::ios::app) < 0)
    {
        return -1;
    }

    return 0;
}

int removeCreds(const std::wstring& wstrUsername, const std::wstring& wstrPath)
{
    // Read all credentials.
    std::string strAllCreds;

    if (readCreds(strAllCreds, wstrPath) < 0)
    {
        return -1;
    }

    // Convert data.
    std::wstring wstrAllCreds;

    if (toWide(wstrAllCreds, strAllCreds) < 0)
    {
        return -1;
    }

    // Find position [0] and length [1] of entry.
    std::vector<int> vIndices = findEntry(wstrUsername, wstrAllCreds);
    
    if (vIndices[0] == -1 && vIndices[1] == -1)
    {
        return -1;
    }

    // Remove found entry.
    wstrAllCreds.erase(vIndices[0], vIndices[1] + 1);

    // Clear UTF-8 buffer.
    strAllCreds.erase();
    
    // Convert data.
    if (toMultibyte(strAllCreds, wstrAllCreds) < 0)
    {
        return -1;
    }

    // Overwrite file with credentials.
    if (writeCreds(
        (BYTE*)strAllCreds.c_str(),
        strAllCreds.size() - 1, // do not include null byte.
        wstrPath,
        std::ios::out) < 0)
    {
        return -1;
    }

    return 0;
}

int encryptCreds(const std::wstring& wstrTempPath)
{
    // Derive key.
    HCRYPTKEY hKey;
    HCRYPTPROV hProv;

    if (!CryptAcquireContext(
        &hProv,
        NULL,
        NULL,
        PROV_RSA_AES,
        CRYPT_VERIFYCONTEXT
    ))
    {
        showError(L"encryptCreds()::CryptAcquireContext()");
        return -1;
    }

    if (getKey(&hProv, &hKey) < 0)
    {
        CryptReleaseContext(hProv, 0);
        return -1;
    }

    // Read contents of temp file.
    std::string strTemp;

    if (readCreds(strTemp, wstrTempPath) < 0)
    {
        return -1;
    }
    
    // Encrypt data.
    BYTE* pbEncrypted = NULL;
    DWORD dwSize = encrypt(
        hKey,
        &pbEncrypted,
        (BYTE*)strTemp.c_str(),
        strTemp.size() + 1
    );

    if (dwSize < 0)
    {
        CryptDestroyKey(hKey);
        CryptReleaseContext(hProv, 0);
        return -1;
    }

    // Delete current creds file.
    _wremove(wcCredsFilePath);

    // Write encrypted entry into credentials file.
    if (writeCreds(
        pbEncrypted,
        dwSize,
        wcCredsFilePath,
        std::ios::out) < 0)
    {
        CryptDestroyKey(hKey);
        CryptReleaseContext(hProv, 0);
        return -1;
    }

    // Delete temp file.
    _wremove(wstrTempPath.c_str());

    // Cleanup.
    CryptDestroyKey(hKey);
    CryptReleaseContext(hProv, 0);

    return 0;
}

int decryptCreds(std::wstring& wstrTempPath)
{
    // Derive key.
    HCRYPTPROV hProv;

    if (!CryptAcquireContext(
        &hProv,
        NULL,
        NULL,
        PROV_RSA_AES,
        CRYPT_VERIFYCONTEXT
    ))
    {
        showError(L"decryptCreds::CryptAcquireContext");
        return -1;
    }

    HCRYPTKEY hKey;

    if (getKey(&hProv, &hKey) < 0)
    {
        CryptReleaseContext(hProv, 0);
        return -1;
    }

    // Read file with encrypted credentials.
    std::string strCreds;

    if (readCreds(strCreds, wcCredsFilePath) < 0)
    {
        return -1;
    }

    // Decrypt data.
    DWORD dwSize = decrypt(hKey, (BYTE*)strCreds.c_str(), strCreds.size());

    if (dwSize < 0)
    {
        CryptDestroyKey(hKey);
        CryptReleaseContext(hProv, 0);
        return -1;
    }

    // Resize buffer to remove not overwritten encrypted data.
    strCreds.resize(dwSize);

    // Remove extra null bytes.
    strCreds.erase(
        std::find(strCreds.begin(), strCreds.end(), '\0'),
        strCreds.end()
    );

    // Create temp file and write decrypted data into it.
    if (writeTemp(wstrTempPath, strCreds) < 0)
    {
        CryptDestroyKey(hKey);
        CryptReleaseContext(hProv, 0);
        return -1;
    }

    // Cleanup.
    CryptDestroyKey(hKey);
    CryptReleaseContext(hProv, 0);

    return 0;
}

int getKey(HCRYPTPROV* phProv, HCRYPTKEY* phKey)
{
    // Get ADMIN passphrase.
    BYTE* pbPassphrase = NULL;

    DWORD dwSize = readRegistryValue(&pbPassphrase, wcPassphraseValue);

    if (dwSize < 0)
    {
        return -1;
    }

    // Create empty hash object.
    HCRYPTHASH hHash;

    if (!CryptCreateHash(*phProv, HASH_ALG, 0, 0, &hHash))
    {
        showError(L"getKey::CryptCreateHash");
        return -1;
    }

    // Add data to created hash object.
    if (!CryptHashData(
        hHash,
        pbPassphrase,
        dwSize,
        0))
    {
        showError(L"getKey::CryptHashData");
        CryptDestroyHash(hHash);
        return -1;
    }

    // Obtain key from password.
    if (!CryptDeriveKey(
        *phProv,
        CRYPT_ALG,
        hHash,
        0,
        phKey
    ))
    {
        showError(L"getKey::CryptDeriveKey");
        CryptDestroyHash(hHash);
        return -1;
    }

    // Set key parameters.
    // Change key encryption mode to ECB.
    BYTE bMode = CRYPT_MODE;

    if (CryptSetKeyParam(*phKey, KP_MODE, &bMode, 0) < 0)
    {
        showError(L"getKey::KP_MODE::CryptSetKeyParam");
        return -1;
    }

    BYTE bPadding = CRYPT_PADDING;

    if (CryptSetKeyParam(*phKey, KP_PADDING, &bPadding, 0) < 0)
    {
        showError(L"getKey::KP_PADDING::CryptSetKeyParam");
        return -1;
    }

    // Cleanup.
    CryptDestroyHash(hHash);

    return 0;
}

DWORD encrypt(HCRYPTKEY hKey, BYTE** ppbCiphertext, const BYTE* pbPlaintext, DWORD dwPTSize)
{
    // Obtain cipher block length.
    DWORD dwDataLen = dwPTSize;
    DWORD dwEncryptedLen = dwDataLen;

    if (!CryptEncrypt(
        hKey,
        NULL,
        TRUE,
        0,
        NULL,
        &dwEncryptedLen,
        0
    ))
    {
        showError(L"encrypt::SIZE::CryptEncrypt");
        return -1;
    }

    // Resize ciphertext buffer.
    if (*ppbCiphertext)
    {
        delete[] *ppbCiphertext;
    }
    *ppbCiphertext = new BYTE[dwEncryptedLen];

    // Copy plaintext to ciphertext buffer.
    memcpy_s(*ppbCiphertext, dwEncryptedLen, pbPlaintext, dwPTSize);

    if (!CryptEncrypt(
        hKey,
        NULL,
        TRUE,
        0,
        *ppbCiphertext,
        &dwDataLen,
        dwEncryptedLen
    ))
    {
        showError(L"encrypt::ENCRYPTION::CryptEncrypt");
        return -1;
    }

    return dwEncryptedLen;
}

DWORD decrypt(HCRYPTKEY hKey, BYTE* pbData, DWORD dwSize)
{
    DWORD dwDataLen = dwSize;

    // Decrypt ciphertext.
    if (!CryptDecrypt(
        hKey,
        NULL,
        TRUE,
        0,
        pbData,
        &dwDataLen
    ))
    {
        showError(L"decrypt::CryptDecrypt");
        return -1;
    }

    return dwDataLen;
}

int isValid(const std::wstring& wstrInput, int bPasswordPolicy)
{
    // Check banned characters.
    for (int i = 0; i < wcslen(wcBanned); i++)
    {
        if (wstrInput.find(wcBanned[i]) != std::wstring::npos)
        {
            return 0;
        }
    }

    // Check password policy if it is set.
    if (bPasswordPolicy)
    {
        bool bLatin = false;
        bool bCyrillic = false;
        bool bDigit = false;
        bool bPunctuation = false;

        for (int i = 0; i < wstrInput.length(); i++)
        {
            std::setlocale(LC_ALL, "en_US.utf8");

            if (std::iswalpha(wstrInput[i]))
            {
                bLatin = true;
            }
            else if (std::iswdigit(wstrInput[i]))
            {
                bDigit = true;
            }
            else if (std::iswpunct(wstrInput[i]))
            {
                bPunctuation = true;
            }

            std::setlocale(LC_ALL, "ukr");

            if (!std::iswalpha(wstrInput[i]))
            {
                bCyrillic = true;
            }
        }

        // Restore locale.
        std::setlocale(LC_ALL, "en_US.utf8");

        return bLatin & bCyrillic & bDigit & bPunctuation;
    }
    
    return 1;
}

int getInfo(std::wstring& wstrInfo)
{
    // General task.
    // Get user name.
    {
        std::wstring wstrUsername(UNLEN + 1, 0);
        DWORD dwSize = UNLEN + 1;

        if (!GetUserName(&wstrUsername[0], &dwSize))
        {
            showError(L"getInfo::GetUserName");
            return -1;
        }

        wstrInfo += wstrUsername + SI_DELIMITER;
    }

    // Get computer name.
    {
        std::wstring wstrComputerName(MAX_COMPUTERNAME_LENGTH + 1, 0);
        DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;

        if (!GetComputerName(&wstrComputerName[0], &dwSize))
        {
            showError(L"getInfo::GetComputerName");
            return -1;
        }

        wstrInfo += wstrComputerName + SI_DELIMITER;
    }

    // Get Windows directory.
    {
        std::wstring wstrWinDirectory(MAX_PATH + 1, 0);

        if (!GetWindowsDirectory(&wstrWinDirectory[0], MAX_PATH))
        {
            showError(L"getInfo::GetWindowsDirectory");
            return -1;
        }

        wstrInfo += wstrWinDirectory + SI_DELIMITER;
    }

    // Get system directory.
    {
        std::wstring wstrSysDirectory(MAX_PATH + 1, 0);

        if (!GetSystemDirectory(&wstrSysDirectory[0], MAX_PATH))
        {
            showError(L"getInfo::GetSystemDirectory");
            return -1;
        }

        wstrInfo += wstrSysDirectory + SI_DELIMITER;
    }

    // Personal task.
    // Get number of mouse buttons.
    {
        int iNumber = GetSystemMetrics(SM_CMOUSEBUTTONS);

        if (!iNumber)
        {
            showError(L"getInfo::CMOUSEBUTTONS::GetSystemMetrics");
            return -1;
        }

        wstrInfo += std::to_wstring(iNumber) + SI_DELIMITER;
    }

    // Get screen width.
    {
        int iWidth = GetSystemMetrics(SM_CXSCREEN);

        if (!iWidth)
        {
            showError(L"getInfo::SM_CXSCREEN::GetSystemMetrics");
            return -1;
        }

        wstrInfo += std::to_wstring(iWidth) + SI_DELIMITER;
    }

    // Get drives.
    {
        std::wstring wstrDrives(BUF_SIZE + 1, 0);
        int iResult = GetLogicalDriveStrings(BUF_SIZE, &wstrDrives[0]);

        if (!iResult || iResult > BUF_SIZE)
        {
            showError(L"getInfo::GetLogicalDriveStrings");
            return -1;
        }

        // Replace null bytes between drive strings with another wchar_t.
        for (int i = 0; i < iResult; i++)
        {
            if (wstrDrives[i] == L'\0')
            {
                wstrDrives[i] = L',';
            }
        }

        wstrInfo += wstrDrives + SI_DELIMITER;
    }

    // Get serial number of drive.
    {
        DWORD dwSerialNumber;

        if (!GetVolumeInformation(NULL, NULL, 0, &dwSerialNumber, NULL, NULL, NULL, 0))
        {
            showError(L"getInfo::GetVolumeInformation");
            return -1;
        }

        wstrInfo += std::to_wstring(dwSerialNumber) + SI_DELIMITER;
    }

    return 0;
}

int readRegistryValue(BYTE** ppbData, const std::wstring& wstrValue)
{
    // Open registry key HKEY_CURRENT_USER\Software\<Student's_Surname>.
    HKEY hkey;

    if (RegOpenKeyEx(
        HKEY_CURRENT_USER, wcRegistryKey, // specify key path
        0,                 // reserved                         
        KEY_QUERY_VALUE,   // get read access
        &hkey              // pointer to key handle
    ))
    {
        showError(L"readRegistryValue::RegOpenKeyEx");
        return -1;
    }

    // Get size of buffer needed to contain value data.
    DWORD dwSize = 1;

    if (RegGetValue(
        hkey,
        L"",
        wstrValue.c_str(),
        RRF_RT_REG_BINARY,
        NULL,
        NULL,
        &dwSize
    ) != ERROR_SUCCESS)
    {
        showError(L"readRegistryValue::SIZE::RegGetValue");
        RegCloseKey(hkey);
        return -1;
    }

    // Resize buffer.
    if (*ppbData)
    {
        delete[] *ppbData;
    }
    *ppbData = new BYTE[dwSize];

    // Get value data.
    if (RegGetValue(
        hkey,
        L"",
        wstrValue.c_str(),
        RRF_RT_REG_BINARY,
        NULL,
        *ppbData,
        &dwSize
    ))
    {
        showError(L"readRegistryValue::VALUE::RegGetValue");
        RegCloseKey(hkey);
        return -1;
    }
    
    // Cleanup.
    RegCloseKey(hkey);

    return dwSize;
}

int checkSignature(const BYTE* pbSignature, DWORD dwSize, const wchar_t* pwcData)
{
    // Initialize cryptography provider.
    HCRYPTPROV hProv;

    if (!CryptAcquireContext(
        &hProv,
        L"SecurityAppContainer",
        NULL,
        PROV_RSA_FULL,
        0
    ))
    {
        showError(L"checkSignature::CryptAcquireContext");
        return -1;
    }

    // Create empty hash object.
    HCRYPTHASH hHash;

    if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
    {
        showError(L"checkSignature::CryptCreateHash");
        CryptReleaseContext(hProv, 0);
        return -1;
    }

    // Add data to created hash object.
    if (!CryptHashData(
        hHash,
        (BYTE*)pwcData,
        (DWORD)(wcslen(pwcData) + 1) * sizeof(wchar_t),
        0
    ))
    {
        showError(L"checkSignature::CryptHashData");
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return -1;
    }

    // Get user public key.
    HCRYPTKEY hUserKey;

    if (!CryptGetUserKey(hProv, AT_SIGNATURE, &hUserKey))
    {
        showError(L"checkSignature::CryptGetUserKey");
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return -1;
    }

    // Verify digital signature.
    if (!CryptVerifySignature(
        hHash,
        pbSignature,
        dwSize,
        hUserKey,
        NULL,
        0
    ))
    {
        showError(L"checkSignature::CryptVerifySignature");
        CryptDestroyKey(hUserKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return -1;
    }

    // Cleanup.
    CryptDestroyKey(hUserKey);
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);

    return 0;
}

int toWide(std::wstring& wstrWide, const std::string& strMultibyte)
{
    // Get necessary buffer size.
    int iSize = MultiByteToWideChar(
        CP_UTF8,
        0,
        &strMultibyte[0],
        -1,
        NULL,
        0
    );

    if (!iSize)
    {
        showError(L"toWide::SIZE::MultiByteToWideChar");
        return -1;
    }

    // Convert UTF-8 (multibyte) characters to wide characters.
    wstrWide.resize(iSize);

    if (!MultiByteToWideChar(
        CP_UTF8,
        0,
        &strMultibyte[0],
        -1,
        &wstrWide[0],
        iSize))
    {
        showError(L"toWide::CONVERSION::MultiByteToWideChar");
        return -1;
    }

    return 0;
}

int toMultibyte(std::string& strMultibyte, const std::wstring& wstrWide)
{
    // Get necessary buffer size.
    int iSize = WideCharToMultiByte(
        CP_UTF8,
        0,
        &wstrWide[0],
        -1,
        NULL,
        0,
        NULL,
        NULL
    );
    if (!iSize)
    {
        showError(L"toMultibyte::SIZE::WideCharToMultiByte");
        return -1;
    }

    // Convert wide characters to UTF-8 (multibyte) characters.
    strMultibyte.resize(iSize);

    if (!WideCharToMultiByte(
        CP_UTF8,
        0,
        &wstrWide[0],
        -1,
        &strMultibyte[0],
        iSize,
        NULL,
        NULL))
    {
        showError(L"toMultibyte::CONVERSION::WideCharToMultiByte");
        return -1;
    }

    return 0;
}

int writeTemp(std::wstring& wstrFilePath, const std::string& strData)
{
    // Create buffer for temp directory path.
    std::wstring wstrDirPath(MAX_PATH + 1, 0);

    if (!GetTempPath(MAX_PATH + 1, &wstrDirPath[0]))
    {
        showError(L"createTemp()::GetTempPath()");
        return -1;
    }

    // Resize buffer for file path.
    wstrFilePath.resize(MAX_PATH + 1);

    if (!GetTempFileName(&wstrDirPath[0], L"CRED", 0, &wstrFilePath[0]))
    {
        showError(L"createTemp()::GetTempFileName()");
        return -1;
    }

    // Write data into temp file.
    if (writeCreds(
        (BYTE*)strData.c_str(),
        strData.size(),
        wstrFilePath,
        std::ios::out) < 0)
    {
        return -1;
    }

    return 0;
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

    wssValue << aValue << std::endl;

    std::wstring wsValue = wssValue.str();

    MessageBox(NULL, wsValue.c_str(), wstrName.c_str(), 0);
}