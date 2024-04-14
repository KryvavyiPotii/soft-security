#pragma once

#include <windows.h>
#include <Lmcons.h>
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <clocale>
#include <cwctype>

// Window classes.
#define LOGIN_CLASS     L"Login class"
#define ADMIN_CLASS     L"Admin class"
#define ADD_CLASS       L"User add class"
#define EDIT_CLASS      L"User edit class"
#define USER_CLASS      L"User class"
#define CHPASS_CLASS    L"Password change class"

// Offsets for adding window elements.
#define ELEMENT_OFFSET  10
#define TEXT_WIDTH      150
#define ELEMENT_HEIGHT  ELEMENT_OFFSET * 2
#define LOGIN_WIDTH		TEXT_WIDTH + ELEMENT_OFFSET * 2
#define LOGIN_HEIGHT	ELEMENT_OFFSET * 17
#define ADMIN_WIDTH		TEXT_WIDTH * 2 + ELEMENT_OFFSET * 2
#define ADMIN_HEIGHT	ELEMENT_OFFSET * 22
#define USER_WIDTH		TEXT_WIDTH + ELEMENT_OFFSET * 2
#define USER_HEIGHT		ELEMENT_OFFSET * 13
#define CHPASS_WIDTH	TEXT_WIDTH + ELEMENT_OFFSET * 2
#define CHPASS_HEIGHT	ELEMENT_OFFSET * 24
#define ADD_WIDTH       TEXT_WIDTH + ELEMENT_OFFSET * 2
#define ADD_HEIGHT      ELEMENT_OFFSET * 18
#define EDIT_WIDTH      TEXT_WIDTH + ELEMENT_OFFSET * 2
#define EDIT_HEIGHT     ELEMENT_OFFSET * 13
#define BUTTON_WIDTH    ELEMENT_OFFSET * 6

// Window identifiers.
#define LOGIN			10
#define LOGIN_USERNAME  11
#define LOGIN_PASSWORD  12
#define LOGIN_CLOSE     13
#define CHPASS_CREATE   20
#define CHPASS_CHANGE   21
#define CHPASS_CANCEL   22
#define CHPASS_CLOSE    23
#define CHPASS_CURRENT  24
#define CHPASS_NEW      25
#define CHPASS_CONFIRM  26
#define MENU_ABOUT      30
#define ADMIN_USERNAME  40
#define ADMIN_ADD       41
#define ADMIN_EDIT      42
#define ADMIN_LIST      43
#define ADMIN_PERMITTED 44
#define ADMIN_PWPOLICY  45
#define USER_CLOSE      50
#define ADD_ADD         60
#define ADD_CANCEL      61
#define ADD_USERNAME    62
#define ADD_PERMITTED   63
#define ADD_PWPOLICY    64
#define EDIT_SET        70
#define EDIT_CANCEL     71
#define EDIT_GETUSER    72
#define EDIT_PERMITTED  73
#define EDIT_PWPOLICY   74

// Identifiers for WM_COPYDATA communication between windows.
#define SRID_USERNAME   1
#define SRID_TEMPPATH   2

// Security.
#define ADMIN_NAME        L"ADMIN"
#define CREDS_DELIMITER   L"$"
#define SI_DELIMITER      L"$"
#define CREDS_FORMAT      L"%s" CREDS_DELIMITER L"%s" CREDS_DELIMITER L"%d" CREDS_DELIMITER L"%d"
#define LOGIN_TRIES       3
#define USERNAME_SIZE     32
#define PASSWORD_SIZE     60
#define BUF_SIZE          1024
#define USER_BLOCKED      0
#define USER_PERMITTED    1 
#define USER_NO_PWPOLICY  0
#define USER_PWPOLICY     1
#define USER_INVALIDVALUE 2

// Cryptography.
#define CRYPTO_PROV     PROV_RSA_AES
#define HASH_ALG        CALG_MD4
#define CRYPT_ALG       CALG_AES_256
#define CRYPT_MODE      CRYPT_MODE_ECB
#define CRYPT_PADDING   PKCS5_PADDING

// Struct that stores user credentials and parameters.
typedef struct User
{
    std::wstring wstrUsername;
    std::wstring wstrPassword;
    int bStatus = USER_INVALIDVALUE;
    int bPasswordPolicy = USER_INVALIDVALUE;
};

// Struct that stores current/last logged user and their number of login tries.
typedef struct CurrentUser
{
    std::wstring wstrUsername;
    int iTries = LOGIN_TRIES;
};

// GUI functions.
// Register all types of windows.
// Arguments:
//     [in] hInstance - handle to instance of program.
// Return value:
//     Success - 0.
//     Failure - -1.
int registerChildren(HINSTANCE hInstance);
// Login window procefure.
LRESULT CALLBACK LoginProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
// ADMIN window procedure.
LRESULT CALLBACK AdminProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
// Non-privileged user window procedure.
LRESULT CALLBACK UserProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
// Change password dialog procedure.
LRESULT CALLBACK ChpassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
// Add user dialog procedure.
LRESULT CALLBACK AddUserProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
// Edit user dialog procedure.
LRESULT CALLBACK EditUserProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
// Add controls to login window.
// Arguments:
//     [in] hwndLogin - handle to login window.
// Return value:
//     Success - 0.
//     Failure - -1.
int addLoginControls(HWND hwndLogin);
// Add controls to ADMIN window.
// Arguments:
//     [in] hwndAdmin - handle to ADMIN window.
// Return value:
//     Success - 0.
//     Failure - -1.
int addAdminControls(HWND hwndAdmin);
// Add controls to non-privileged user window.
// Arguments:
//     [in] hwndUser - handle to non-privileged user window.
// Return value:
//     Success - 0.
//     Failure - -1.
int addUserControls(HWND hwndUser);
// Add controls to password change window.
// Arguments:
//     [in] hwndChpass - handle to password change window.
// Return value:
//     Success - 0.
//     Failure - -1.
int addChpassControls(HWND hwndChpass);
// Add controls to user add window.
// Arguments:
//     [in] hwndAddUser - handle to user add window.
// Return value:
//     Success - 0.
//     Failure - -1.
int addAddUserControls(HWND hwndAddUser);
// Add controls to user edit window.
// Arguments:
//     [in] hwndEditUser - handle to user edit window.
// Return value:
//     Success - 0.
//     Failure - -1.
int addEditUserControls(HWND hwndEditUser);
// Add menu bar to user windows.
// Arguments:
//     [in] hwnd - handle to user window.
// Return value:
//     Success - 0.
//     Failure - -1.
int addMenu(HWND hwnd);
// Create password change dialog.
// Arguments:
//     [in] hwndOwner - handle to owner window (ADMIN or non-privileged user).
//     [in] hInstance - handle to instance of program.
// Return value:
//     Success - handle to password change window.
//     Failure - NULL.
HWND createChpassDialog(HWND hwndOwner, HINSTANCE hInstance);
// Create and fill list box with users.
// Arguments:
//     [in] hwndAdmin - handle to ADMIN window.
//     [in] wstrPath - reference to wstring with credentials file path.
// Return value:
//     Success - 0.
//     Failure - -1.
int createListBox(HWND hwndAdmin, const std::wstring& wstrPath);
// Show information about user that is selected in list box.
// Arguments:
//     [in] hwndAdmin - handle to ADMIN window.
//     [in] wstrPath - reference to wstring with credentials file path.
// Return value:
//     Success - 0.
//     Failure - -1.
int showSelectedUserInfo(HWND hwndAdmin, const std::wstring& wstrPath);
// Send data to another window.
// This function is used for sending username and credentials file path.
// Arguments:
//     [in] hwndReceiver - handle to receiver window.
//     [in] hwndSender - handle to sender window.
//     [in] pbData - pointer to BYTE array with data.
//     [in] dwSize - size of array pointed by pbData in bytes.
//     [in] dwID - identifier:
//         - SRID_USERNAME
//         - SRID_TEMPPATH
void sendData(HWND hwndReceiver, HWND hwndSender, const BYTE* pbData, DWORD dwSize, DWORD dwID);

// Security functions.
// Identify user by username.
// Arguments:
//     [in, out] wstrPath - reference to wstring with credentials file path.
//     [in] hwndLogin - handle to login window.
//     [in, out] pCurrentUser - pointer to CurrentUser struct.
//         Before execution it should contain username of the last logged in user and number of his/her login tries.
//         After successful execution it receives new username and updated login tries.
// Return value:
//     Success - 0.
//     Failure - -1.
int identify(std::wstring& wstrPath, HWND hwndLogin, CurrentUser* pCurrentUser);
// Authenticate user by password.
// Arguments:
//     [in, out] wstrPath - reference to wstring that receives path to file with decrypted credentials.
//     [in] hwndLogin - handle to login window.
//     [in, out] pCurrentUser - pointer to CurrentUser struct.
//         Before execution it should contain username of the last logged in user and number of his/her login tries.
//         After successful execution it will receive new username and updated login tries.
// Return value:
//     Success - 0.
//     Failure - -1.
int authenticate(const std::wstring& wstrTempPath, HWND hwndLogin, CurrentUser* pCurrentUser);
// Authorize user by username.
// Creates user window based on user's privileges.
// Arguments:
//     [in] wstrUsername - reference to wstring with username of logged in user.
// Return value:
//     Success - handle to created window.
//     Failure - NULL.
HWND authorize(const std::wstring& wstrUsername);
// Change password of logged in user.
// Arguments:
//     [in] hwndChpass - handle to password change window.
//     [in] hwndOwner - handle to owner window of password change window.
//     [in] wstrUsername - reference to wstring with username of logged in user.
//     [in] wstrPath - reference to wstring with credentials file path.
// Return value:
//     Success - 0.
//     Failure - -1.
int changePassword(HWND hwndChpass, HWND hwndOwner, const std::wstring& wstrUsername, const std::wstring& wstrPath);
// Add new user to system with empty password and specified parameters.
// Arguments:
//     [in] hwndAddUser - handle to user add window.
//     [in] hwndOwner - handle to owner window of password change window.
//     [in] wstrPath - reference to wstring with credentials file path.
// Return value:
//     Success - 0.
//     Failure - -1.
int addUser(HWND hwndAddUser, HWND hwndOwner, const std::wstring& wstrPath);
// Edit selected in list box user's parameters.
// Arguments:
//     [in] hwndEditUser - handle to user edit window.
//     [in] hwndOwner - handle to owner window of user edit window.
//     [in] wstrPath - reference to wstring with credentials file path.
// Return value:
//     Success - 0.
//     Failure - -1.
int editUser(HWND hwndEditUser, HWND hwndOwner, const std::wstring& wstrPath);
// Create file with default credentials and encrypt it.
// Return value:
//     Success - 0.
//     Failure - -1.
int createCreds();
// Read contents of credentials file.
// File is expected to have UTF-8 encoding.
// Arguments:
//     [in, out] strCreds - reference to string that receives credentials.
//     [in] wstrPath - reference to wstring with credentials file path.
// Return value:
//     Success - 0.
//     Failure - -1.
int readCreds(std::string& strCreds, const std::wstring& wstrPath);
// Write data (credentials) to file.
// File is expected to have UTF-8 encoding.
// Arguments:
//     [in] pbData - pointer to BYTE array with data.
//     [in] dwSize - size of array pointed by pbData in bytes.
//     [in] wstrPath - reference to wstring with credentials file path.
//     [in] mode - mode in which binary file will be opened.
//         In function it is ORed with std::ios::binary.
// Return value:
//     Success - 0.
//     Failure - -1.
int writeCreds(const BYTE* pbCreds, DWORD dwSize, const std::wstring& wstrPath, std::ios::openmode mode);
// Find and store credentials of specified user.
// Arguments:
//     [in, out] pUser - pointer to User struct.
//         Username is necessary for finding user's entry.
//         After successful execution it will contain full credentials (by parseCreds()).
//     [in] wstrPath - reference to wstring with path to credentials file.
// Return value:
//     Success - 0.
//     Failure - -1.
int getCreds(User* pUser, const std::wstring& wstrPath);
// Create entry wstring separated with CREDS_DELIMITER.
// Arguments:
//     [in, out] pUser - pointer to User struct.
//         After execution extra null bytes in username and password will be removed.
// Return value: wstring with entry.
//     Success or failure should be tested with another function.
std::wstring createEntry(User* pUser);
// Find position and length of user credentials entry.
// Arguments:
//     [in] wstrUsername - reference to wstring with username.
//     [in] wstrAllCreds - referense to wstring with all credentials.
// Return value:
//     Success - 2D vector { position of entry; length of entry }.
//     Failure - 2D vector { -1; -1 }.
std::vector<int> findEntry(const std::wstring& wstrUsername, const std::wstring& wstrAllCreds);
// Parse credentials of specified user.
// Arguments:
//     [in, out] pUser - pointer to User struct.
//         After successful execution it will contain full credentials.
//     [in] wstrEntry - reference to wstring with user credentials.
// Return value:
//     Success - 0.
//     Failure - -1.
int parseEntry(User* pUser, const std::wstring& wstrEntry);
// Add or modify credentials of specified user in credentials file.
// Arguments:
//     [in, out] pUser - pointer to User struct with credentials.
//         After execution extra null bytes in username and password will be removed.
//     [in] wstrPath - reference to wstring with credentials file path.
// Return value:
//     Success - 0.
//     Failure - -1.
int setCreds(User* pUser, const std::wstring& wstrPath);
// Remove credentials of specified user from credentials file.
// Arguments:
//     [in] wstrUsername - reference to wstring with username.
//     [in] wstrPath - reference to wstring with credentials file path.
// Return value:
//     Success - 0.
//     Failure - -1.
int removeCreds(const std::wstring& wstrUsername, const std::wstring& wstrPath);
// Encrypt temp file and write encrypted data into credentials file.
// Arguments:
//     [in] wstrTempPath - reference to wstring with credentials file path.
// Return value:
//     Success - 0.
//     Failure - -1.
int encryptCreds(const std::wstring& wstrTempPath);
// Decrypt credentials file and write decrypted data into temp file.
// Arguments:
//     [in, out] wstrTempPath - reference to wstring that receives temp file path.
// Return value:
//     Success - 0.
//     Failure - -1.
int decryptCreds(std::wstring& wstrTempPath);
// Create key derived from ADMIN passphrase for CRYPT_ALG encryption in CRYPT_MODE mode.
// Arguments:
//     [in] phProv - pointer to initialized cryptoprovider with type CRYPTO_PROV.
//     [in, out] phKey - pointer to HCRYPTKEY that receives derived from passphrase key.
// Return value:
//     Success - 1.
//     Failure - 0.
int getKey(HCRYPTPROV* phProv, HCRYPTKEY* phKey);
// Encrypt plaintext with a key.
// Caller is responsible for cryptoprovider initialization and release.
// Arguments:
//     [in] hKey - cryptographic key for encryption.
//     [in, out] ppbCiphertext - double pointer to BYTE array that receives ciphertext.
//     [in] pbPlaintext - pointer to BYTE array with plaintext.
//     [in] dwPTSize - size of plaintext in bytes.
// Return value:
//     Success - size of encrypted data in bytes.
//     Failure - -1.
DWORD encrypt(HCRYPTKEY hKey, BYTE** ppbCiphertext, const BYTE* pbPlaintext, DWORD dwPTSize);
// Decrypt ciphertext with a key.
// Caller is responsible for cryptoprovider initialization and release.
// Arguments:
//     [in] hKey - cryptographic key for encryption.
//     [in, out] pbData - double pointer to BYTE array.
//         Before execution it contains ciphertext.
//         After successful execution it will receive decrypted plaintext.
//     [in] dwPTSize - size of BYTE array pointed by pbData in bytes.
// Return value:
//     Success - size of encrypted data in bytes.
//     Failure - -1.
DWORD decrypt(HCRYPTKEY hKey, BYTE* pbData, DWORD dwSize);
// Check entered credentials for banned characters and password policy violation if necessary.
// Arguments:
//     [in] wstrInput - reference to wstring with credentials.
//     [in] bPasswordPolicy - flag:
//         - USER_NO_PWPOLICY - password policy will not be checked.
//         - USER_PWPOLICY - password policy will be checked.
// Return value:
//     Success - 1.
//     Failure - 0.
int isValid(const std::wstring& wstrInput, int bPasswordPolicy);
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
// Read values from HKEY_CURRENT_USER\Software\<Student_Surname>\.
// Arguments:
//     [in, out] ppbData - double pointer to BYTE array that receives value data.
//     [in] wstrValue - reference to wstring with value title.
// Return value:
//     Success - 0.
//     Failure - -1.
int readRegistryValue(BYTE** ppbData, const std::wstring& wstrValue);
// Verify signature.
// Arguments:
//     [in] pbSignature - pointer to BYTE array that contains signed data.
//     [in] dwSize - size of array pointed by pbSignature in bytes.
//     [in] pwcData - pointer to wchar_t array with data upon which signature is verified.
//         In our case, it is system info.
// Return value:
//     Success - 0.
//     Failure - -1 (if signature is not valid or if error occured).
int checkSignature(const BYTE* pbSignature, DWORD dwSize, const wchar_t* pwcData);

// Auxiliary functions.
// Convert UTF-8 string to wide character string.
// Arguments:
//     [in, out] wstrWide - reference to wstring that receives converted data.
//     [in] strMultibyte - reference to string with UTF-8 data.
// Return value:
//     Success - 0.
//     Failure - -1.
int toWide(std::wstring& wstrWide, const std::string& strMultibyte);
// Convert wide character string to UTF-8 string.
// Arguments:
//     [in, out] strMultibyte - reference to string that receives converted data.
//     [in] wstrWide - reference to wstring.
// Return value:
//     Success - 0.
//     Failure - -1.
int toMultibyte(std::string& strMultibyte, const std::wstring& wstrWide);
// Create temp file and write data into it.
// Arguments:
//     [in, out] wstrFilePath - reference to wstring that receives temp file path.
//     [in] pbData - pointer to BYTE array with data to write.
// Return value:
//     Success - 0.
//     Failure - -1.
int writeTemp(std::wstring& wstrFilePath, const std::string& strData);

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