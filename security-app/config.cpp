#include "config.h"

const wchar_t wcRegistryKey[] = L"Software\\Surname";
const wchar_t wcSignatureValue[] = L"Signature";
const wchar_t wcPassphraseValue[] = L"Passphrase";
const wchar_t wcAboutProgram[] = L"Author: KryvavyiPotii\n\n"
L"This configuration program is created for fighting against illegal copying of SecurityApp.\n";

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    // Class creation and registration.
    WNDCLASSEX wcx = { 0 };

    wcx.cbSize = sizeof(wcx);
    wcx.lpfnWndProc = ConfigProc;
    wcx.hInstance = hInstance;
    wcx.lpszClassName = CONFIG_CLASS;
    wcx.hIcon = LoadIcon(NULL, IDI_SHIELD);

    if (!RegisterClassEx(&wcx))
    {
        showError(L"wWinMain::RegisterClassEx");
        return -1;
    }

    // Create the window.
    HWND hwnd = CreateWindowEx(
        0, CONFIG_CLASS, L"Configuration",
        WS_OVERLAPPED | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CONFIG_WIDTH, CONFIG_HEIGHT,
        NULL, NULL, hInstance, NULL
    );
    if (hwnd == NULL)
    {
        showError(L"wWinMain::CreateWindowEx");
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

LRESULT CALLBACK ConfigProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        addMenu(hwnd);
        addControls(hwnd);
        return 0;

    case WM_CLOSE:
    {
        // Ask user before exiting program.
        if (MessageBox(
            NULL,
            L"You may configure app another time.\n\n"
            L"Exit Configuration?",
            L"Exit Configuration",
            MB_YESNO | MB_ICONQUESTION
        ) == IDYES)
        {
            DestroyWindow(hwnd);
        }

        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case CONFIG_OK:
        {

            if (configure(hwnd) < 0)
            {
                break;
            }

            // Exit Configuration.
            MessageBox(
                NULL,
                L"App was successfully configured.\n"
                L"Press \"OK\" to exit Configuration.",
                L"Configuration finished",
                MB_OK | MB_ICONINFORMATION
            );

            DestroyWindow(hwnd);

            break;
        }

        case CONFIG_CANCEL:
            DestroyWindow(hwnd);
            break;

        // Show information about program.
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

int addControls(HWND hwndConfig)
{
    if (!CreateWindowEx(
        0, L"STATIC", L"Enter passphrase:",
        WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET,
        TEXT_WIDTH - ELEMENT_OFFSET * 2,
        ELEMENT_HEIGHT,
        hwndConfig, NULL, NULL, NULL
    ))
    {
        showError(L"addControls::STATIC::PASSPHRASE");
        return -1;
    }

    if (!CreateWindowEx(
        0, L"EDIT", L"",
        ES_AUTOHSCROLL | WS_BORDER | WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        ELEMENT_OFFSET * 3,
        TEXT_WIDTH - ELEMENT_OFFSET * 2,
        ELEMENT_HEIGHT,
        hwndConfig, (HMENU)CONFIG_PASS, NULL, NULL
    ))
    {
        showError(L"addControls::EDIT::PASSPHRASE");
        return -1;
    }

    if (!CreateWindowEx(
        0, L"BUTTON", L"OK",
        WS_BORDER | WS_CHILD | WS_VISIBLE,
        ELEMENT_OFFSET,
        CONFIG_HEIGHT - ELEMENT_OFFSET * 9,
        BUTTON_WIDTH,
        ELEMENT_HEIGHT,
        hwndConfig, (HMENU)CONFIG_OK, NULL, NULL
    ))
    {
        showError(L"addControls::BUTTON::OK");
        return -1;
    }

    if (!CreateWindowEx(
        0, L"BUTTON", L"Cancel",
        WS_BORDER | WS_CHILD | WS_VISIBLE,
        CONFIG_WIDTH - BUTTON_WIDTH - ELEMENT_OFFSET * 3,
        CONFIG_HEIGHT - ELEMENT_OFFSET * 9,
        BUTTON_WIDTH,
        ELEMENT_HEIGHT,
        hwndConfig, (HMENU)CONFIG_CANCEL, NULL, NULL
    ))
    {
        showError(L"addControls::BUTTON::CANCEL");
        return -1;
    }

    return 0;
}

int addMenu(HWND hwndConfig)
{
    // Create and add menu bar.
    HMENU hMenuBar = CreateMenu();
    HMENU hMenuHelp = CreateMenu();

    if (!hMenuBar || !hMenuHelp)
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

    // Add created options to menu bar.
    if (!AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hMenuHelp, L"Help"))
    {
        showError(L"addMenu::HELP::AppendMenu");
        return -1;
    }

    if (!SetMenu(hwndConfig, hMenuBar))
    {
        showError(L"addMenu::SetMenu");
        return -1;
    }

    return 0;
}

int configure(HWND hwndConfig)
{
    // Get ADMIN passphrase.
    BYTE* pbPassphrase = NULL;
    DWORD dwPassSize;

    {
        // Get handle to passphrase field.
        HWND hwndPassphrase = GetDlgItem(hwndConfig, CONFIG_PASS);

        if (hwndPassphrase == NULL)
        {
            showError(L"configure::GetDlgItem");
            return -1;
        }

        // Get size for passphrase buffer in wide characters.
        DWORD dwPassSizeW = GetWindowTextLength(hwndPassphrase);

        if (!dwPassSizeW)
        {
            if (!GetLastError())
            {
                MessageBox(
                    NULL,
                    L"Passphrase is not specified.",
                    L"Alert",
                    MB_OK | MB_ICONWARNING
                );
            }
            else
            {
                showError(L"configure::GetWindowTextLength");
            }
            return -1;
        }

        // Get size for passphrase buffer in bytes.
        // Null byte is not included.
        dwPassSize = (dwPassSizeW) * sizeof(wchar_t);

        // Allocate memory for passphrase buffer.
        pbPassphrase = new BYTE[dwPassSize];

        // Get passphrase.
        if (!GetWindowText(hwndPassphrase, (wchar_t*)pbPassphrase, dwPassSizeW + 1))
        {
            showError(L"configure::GetWindowText");
            return -1;
        }
    }

    // Store ADMIN passphrase in Passphrase value.
    if (writeRegistryValue(pbPassphrase, dwPassSize, wcPassphraseValue) < 0)
    {
        delete[] pbPassphrase;
        return -1;
    }

    // Cleanup.
    delete[] pbPassphrase;

    // Get system info.
    std::wstring wstrInfo;

    if (getInfo(wstrInfo) < 0)
    {
        return -1;
    }

    // Hash system info.
    BYTE* pbSignature = NULL;
    DWORD dwSignSize = createSignature(&pbSignature, wstrInfo.c_str());

    if (dwSignSize <= 0)
    {
        return -1;
    }

    // Store hashed data in Signature value.
    if (writeRegistryValue(pbSignature, dwSignSize, wcSignatureValue) < 0)
    {
        delete[] pbSignature;
        return -1;
    }

    // Cleanup.
    delete[] pbSignature;

    return 0;
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

DWORD createSignature(BYTE** ppbSignature, const wchar_t* pwcData)
{
    HCRYPTPROV hProv;

    // Initialize cryptography provider.
    if (!CryptAcquireContext(
        &hProv,
        L"SecurityAppContainer",
        NULL,
        PROV_RSA_FULL,
        CRYPT_NEWKEYSET
    ))
    {
        // If container exists, delete it and create new.
        if (GetLastError() == NTE_EXISTS)
        {
            if (CryptAcquireContext(
                &hProv,
                L"SecurityAppContainer",
                NULL,
                PROV_RSA_FULL,
                CRYPT_DELETEKEYSET
            ) == ERROR_BUSY)
            {
                showError(L"createSignature::DELETEKEYSET::CryptAcquireContext");
                CryptReleaseContext(hProv, 0);
                return -1;
            }

            if (!CryptAcquireContext(
                &hProv,
                L"SecurityAppContainer",
                NULL,
                PROV_RSA_FULL,
                CRYPT_NEWKEYSET
            ))
            {
                showError(L"createSignature::NEWKEYSET::CryptAcquireContext");
                CryptReleaseContext(hProv, 0);
                return -1;
            }
        }
        else 
        {
            showError(L"createSignature::CryptAcquireContext");
            CryptReleaseContext(hProv, 0);
            return -1;
        }
    }

    // Generate key pair for digital signature.
    HCRYPTKEY hKey;

    if (!CryptGenKey(hProv, AT_SIGNATURE, 0, &hKey))
    {
        showError(L"createSignature::CryptGenKey");
        CryptReleaseContext(hProv, 0);
        return -1;
    }

    // Cleanup.
    CryptDestroyKey(hKey);

    // Create empty hash object.
    HCRYPTHASH hHash;

    if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
    {
        showError(L"createSignature::CryptCreateHash");
        CryptReleaseContext(hProv, 0);
        return -1;
    }

    if (!CryptHashData(
        hHash,
        (BYTE*)pwcData,
        (DWORD)(wcslen(pwcData) + 1) * sizeof(wchar_t),
        0
    ))
    {
        showError(L"createSignature::CryptHashData");
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return -1;
    }

    // Determine size of signature.
    DWORD dwSize;

    if (!CryptSignHash(
        hHash,
        AT_SIGNATURE,
        NULL,
        0,
        NULL,
        &dwSize
    ))
    {
        showError(L"createSignature::CryptSignHash");
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return -1;
    }

    // Resize buffer for signed data.
    if (*ppbSignature)
    {
        delete[] *ppbSignature;
    }
    *ppbSignature = new BYTE[dwSize];

    // Create signature.
    if (!CryptSignHash(
        hHash,
        AT_SIGNATURE,
        NULL,
        0,
        *ppbSignature,
        &dwSize
    ))
    {
        showError(L"createSignature::CryptSignHash");
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return -1;
    }

    // Cleanup.
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);

    return dwSize;
}

int writeRegistryValue(const BYTE* pbData, DWORD dwSize, const std::wstring& wstrValue)
{
    // Create or open registry key HKEY_CURRENT_USER\Software\<Student's_Surname>.
    HKEY hkey;
    DWORD dwDisposition;

    if (RegCreateKeyEx(
        HKEY_CURRENT_USER, wcRegistryKey, // specify key path
        0,                       // reserved
        NULL,                    // no specific class needed                            
        REG_OPTION_NON_VOLATILE, // value should be preserved on system restart
        KEY_ALL_ACCESS,          // give all access.
        NULL,                    // no specific security attributes needed
        &hkey,                   // pointer to key handle
        &dwDisposition           // check if key does not exist
    ))
    {
        showError(L"writeRegistryValue::RegCreateKeyEx");
        return -1;
    }

    // Set value to opened key.
    if (RegSetValueEx(
        hkey,
        wstrValue.c_str(),
        0,
        REG_BINARY,
        pbData,
        dwSize
    ))
    {
        showError(L"writeRegistryValue::RegSetValueEx");
        RegCloseKey(hkey);
        return -1;
    }

    // Cleanup.
    RegCloseKey(hkey);

    return 0;
}

void showError(std::wstring wstrError)
{
    std::wstringstream wsstr;

    wsstr << wstrError << L". Error: " << GetLastError()
        << L" (0x" << std::hex << GetLastError() << L")" << std::endl;

    std::wstring wstr = wsstr.str();

    MessageBox(NULL, wstr.c_str(), L"Error", MB_OK);
}