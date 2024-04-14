#pragma once

#include <windows.h>
#include <Lmcons.h>
#include <shlobj_core.h>
#include <AclAPI.h>
#include <Wincrypt.h>
#include <sstream>
#include <vector>

// Window classes.
#define CONFIG_CLASS L"Configuration class"

// Offsets for adding window elements.
#define ELEMENT_OFFSET  10
#define TEXT_WIDTH      150
#define ELEMENT_HEIGHT  ELEMENT_OFFSET * 2
#define CONFIG_WIDTH    TEXT_WIDTH + ELEMENT_OFFSET * 2
#define CONFIG_HEIGHT	ELEMENT_OFFSET * 15
#define BUTTON_WIDTH    ELEMENT_OFFSET * 6

// Window identifiers.
#define CONFIG_PASS     10
#define CONFIG_OK       11
#define CONFIG_CANCEL   12
#define MENU_ABOUT      20

// Extra constants.
#define SI_DELIMITER    L"$"
#define BUF_SIZE        1024

// GUI functions.
// Configuration window procedure.
LRESULT CALLBACK ConfigProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
// Add controls to configuration window.
// Arguments:
//     [in] hwnd - handle to configuration window.
// Return value:
//     Success - 0.
//     Failure - -1.
int addControls(HWND hwndConfig);
// Add menu bar to configuration window.
// Arguments:
//     [in] hwnd - handle to configuration window.
// Return value:
//     Success - 0.
//     Failure - -1.
int addMenu(HWND hwndConfig);

// Security functions.
// Write passphrase and signed hash of system information into registry.
// Registry key is HKEY_CURRENT_USER\Software\<Student's_Surname>\.
// Arguments:
//     [in] hwndConfig - handle to configuration window.
// Return value:
//     Success - 0.
//     Failure - -1.
int configure(HWND hwndConfig);
// Get system information:
// - User name;
// - Computer name;
// - Windows directory;
// - System directory;
// - Number of mouse buttons;
// - Screen width;
// - Drives;
// - Serial number of drive.
// Arguments:
//     [in, out] wstrInfo - reference to string that receives system information.
//         Information is separated with SI_DELIMITER.
// Return value:
//     Success - 0.
//     Failure - -1.
int getInfo(std::wstring& wstrInfo);
// Hash and sign data.
// Arguments:
//     [in, out] ppbSignature - double pointer to BYTE array.
//         After successful execution it receives signed hash of data.
//     [in] pwcData - pointer to wchar_t array with data.
//         In our case it is system information.
// Return value:
//     Success - size of pbSignature in bytes.
//     Failure - -1.
DWORD createSignature(BYTE** ppbSignature, const wchar_t* pwcData);
// Store data in value of HKEY_CURRENT_USER\Software\<Student's_Surname>\.
// Arguments:
//     [in] pbData - pointer to BYTE array that should be written in registry.
//     [in] dwSize - size of array pointed by pbData.
//     [in] wstrValue - reference to wstring with value title.
// Return value:
//     Success - 0.
//     Failure - -1.
int writeRegistryValue(const BYTE* pbData, DWORD dwSize, const std::wstring& wstrValue);

// Debug functions.
// Show error message box (DEBUGGING).
// Arguments:
//     [in] wstrError - wstring with error message.
void showError(std::wstring wstrError);