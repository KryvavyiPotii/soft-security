#include <windows.h>
#include <sstream>

// Window class.
#define KEYGEN_CLASS L"Key generator class"

// Offsets for adding window elements.
#define ELEMENT_OFFSET  10
#define LINE_WIDTH      ELEMENT_OFFSET * 20
#define ELEMENT_HEIGHT  ELEMENT_OFFSET * 2
#define BUTTON_WIDTH    ELEMENT_OFFSET * 8
#define KEYGEN_WIDTH    LINE_WIDTH + ELEMENT_OFFSET * 2
#define KEYGEN_HEIGHT	ELEMENT_OFFSET * 16

// Control identifiers.
#define CID_USERNAME    10 
#define CID_PASSWORD    11
#define CID_GENERATE    12
#define CID_CANCEL      13
#define MENU_ABOUT      30

// GUI functions.
// Window procedure.
LRESULT CALLBACK KeygenProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
// Add controls to a window (text fields and buttons).
// Arguments:
//     [in] hwnd - a handle to the KeyGen window.
// Return value:
//     Success - 0.
//     Failure - -1.
int addControls(HWND hwnd);
// Add menu bar to a window.
// Arguments:
//     [in] hwnd - a handle to the KeyGen window.
// Return value:
//     Success - 0.
//     Failure - -1.
int addMenu(HWND hwnd);

// Password generation functions.
// Generate a password (key) based on the entered username.
// Arguments:
//     [in] hwnd - a handle to the KeyGen window.
// Return value:
//     Success - 0.
//     Failure - -1.
int keygen(HWND hwnd);
// Derive a password (key) number from a username.
// Arguments:
//     [in] pbUsername - a pointer to a BYTE array with a username.
//     [in, out] puNumber - a pointer to a variable that receives the derived number.
//     [in] iUsernameSize - size of a BYTE array pointed by pbUsername without null terminator.
void deriveNumber(BYTE* pbUsername, UINT* puNumber, int iUsernameSize);
// Create a password based on the derived number.
// Arguments:
//     [in] uNumber - the derived number.
//     [in, out] pbPassword - a pointer to BYTE array that receives the created password.
//         Size of the array must be 9 (8 BYTEs with null terminator).
void createPassword(UINT uNumber, BYTE* pbPassword);

// Debug functions.
// Show error message box.
// Arguments:
//     [in] wstrError - wstring with error message.
void showError(const std::wstring& wstrError);
// Show value of some numeric variable.
// Arguments:
//     [in] wstrName - variable name.
//         However, it can be any message that developer finds comfortable enough.
//     [in] aValue - numeric value that needs to be shown.
void showValue(const std::wstring& wstrName, auto aValue);
