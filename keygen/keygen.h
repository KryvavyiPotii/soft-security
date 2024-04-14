#include <windows.h>
#include <sstream>

#define KEYGEN_CLASS L"Key generator class"

// Offsets for adding window elements.
#define ELEMENT_OFFSET  10
#define LINE_WIDTH      ELEMENT_OFFSET * 20
#define ELEMENT_HEIGHT  ELEMENT_OFFSET * 2
#define BUTTON_WIDTH    ELEMENT_OFFSET * 8
#define KEYGEN_WIDTH    LINE_WIDTH + ELEMENT_OFFSET * 2
#define KEYGEN_HEIGHT	ELEMENT_OFFSET * 15

// Control identifiers.
#define CID_USERNAME    10 
#define CID_PASSWORD    11
#define CID_GENERATE    12
#define CID_CANCEL      13
#define MENU_ABOUT      30

// GUI functions.
LRESULT CALLBACK KeygenProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int addControls(HWND hwnd);
int addMenu(HWND hwnd);

// Password generation functions.
void CreateNum(BYTE* pbUsername, UINT* pNum, int iUsernameSize);
void CreatePassword(UINT uNum, BYTE* pbPassword);

// Debug functions.
// Show error message box.
// Arguments:
//     [in] wstrError - wstring with error message.
void showError(const std::wstring& wstrError);