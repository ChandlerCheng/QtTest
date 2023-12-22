#include "headers/connector.h"

// C++
#include <iostream>
#include <sstream>
#include <exception>

// windows
#include <tchar.h>
#include <setupapi.h>

using std::string;
using std::cout;
using std::cerr;
using std::endl;

////////////
/// base ///
////////////
Connector::Connector()
{

}

Connector::~Connector () noexcept
{

}

int Connector::init(int portNum)
{
    // for com to createfile
    TCHAR lpFileName[64] = {0};

    if(portNum < 10) // COM 1 ~ 9
        swprintf_s(lpFileName, 16, _T("COM%d"), portNum);
    else // COM 10 ~ 256
        swprintf_s(lpFileName, 16, _T("\\\\.\\COM%d"), portNum);

    hDevice = CreateFile(lpFileName,
                         GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                         NULL);

    if (hDevice == INVALID_HANDLE_VALUE) {

        cerr << "CreateFile error code: "<< GetLastError() << endl;

        return 0;
    }

    return 1;
}

int Connector::init(TCHAR* lpFileName)
{
    // for usb to createfile
    if (!*lpFileName) {
        cout << "lpFileName not exists" << endl;
        return false;
    }

    // Use overlapped if enable to do timeout.
    hDevice = CreateFile(lpFileName,
                         GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                         NULL );

    if (hDevice == INVALID_HANDLE_VALUE) {

        cout << "CreateFile error code: "<< GetLastError() << endl;

        return 0;
    }

    return 1;
}

int Connector::init(const std::any& deviceName, bool isOverlapped)
{
    // for usb to createfile
    if (!deviceName.has_value()) {

        cout << "lpFileName not exists" << endl;

        return 0;
    }

    // for com to createfile
    TCHAR lpFileName[64] = {0};

    string deviceNameType(deviceName.type().name());
    if (deviceNameType == "int") {
        swprintf_s(lpFileName, 16, _T("\\\\.\\COM%d"), std::any_cast<int>(deviceName));
    }
    else {
        memcpy(lpFileName, std::any_cast<TCHAR*>(deviceName), 64);
    }

    // Use overlapped if enable to do timeout.
    if (isOverlapped) {
        hDevice = CreateFile(lpFileName,
                             GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                             NULL );
    }
    else {
        hDevice = CreateFile(lpFileName,
                             GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL );
    }

    if (hDevice == INVALID_HANDLE_VALUE) {

        cout << "CreateFile error code: "<< GetLastError() << endl;

        return 0;
    }

    return 1;
}

int Connector::write(QByteArray command)
{
    /*
     * The C string functions use 0 to mark the end of a string.
     * For example: char* cmd = {0x01, 0x02, 0x03, 0x00},
     * strlen(cmd) will stop counting in '\0', so just pick prefix 3 parameters,
     * So need to plus 1 to pick the last value 0x00 whick equal to '\0'.
     * sizeof will get all size of char.
     * const char* of input cmd let strlen get 3 char.
    */
    int nWriteSize = command.size() + 1;
    DWORD NumberOfBytesWrite = 0;
    DWORD NumberOfBytesTransferred = 0;


    OVERLAPPED overlappedWrite;
    // manual-reset event & init state is signaled
    // ref:
    // https://learn.microsoft.com/zh-tw/windows/win32/sync/using-event-objects
    // https://www.cnblogs.com/gaoquanning/p/7249084.html
    // https://learn.microsoft.com/zh-tw/troubleshoot/windows/win32/asynchronous-disk-io-synchronous
    overlappedWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    BOOL isWrite = FALSE;
    try {
        // if the function fails, or is completing asynchronously, the return value is zero(FALSE)
        isWrite = WriteFile(hDevice, command, nWriteSize, &NumberOfBytesWrite, &overlappedWrite);

        if (!isWrite && GetLastError() == ERROR_IO_PENDING) {
            // wait the WriteFile func. in the signaled state or the timeout interval elapses
            int dwRet = WaitForSingleObject(overlappedWrite.hEvent, timeout);
            if (dwRet != WAIT_OBJECT_0) {

                cerr << "write waitForSingleObject error code: " << GetLastError() << endl;

                throw(0);
            }

            // result of overlapped operation
            // bWait in GetOverlappedResult func. is wait infinite, if you want timeout, open the above func. WaitForSingleObject
            if (!GetOverlappedResult(hDevice, &overlappedWrite, &NumberOfBytesTransferred, TRUE)) {

                cerr << "GetOverlappedResult error code: "<< GetLastError() << endl;

                throw(0);
            }
        }
    }
    catch (...) {

//        cerr << e.what() << endl;

        CloseHandle(overlappedWrite.hEvent);
        overlappedWrite.hEvent = NULL;

        return 0;
    }

    // close overlapped handle
    CloseHandle(overlappedWrite.hEvent);
    overlappedWrite.hEvent = NULL;

    return 1;
}

int Connector::read()
{
    int readBufSize = max_buffer_size;
    char* lpbDataBuf = new char[readBufSize]{0};

    DWORD NumberOfBytesRead = 0;
    DWORD NumberOfBytesTransferred = 0;

    OVERLAPPED overlappedRead;
    overlappedRead.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    BOOL isRead = FALSE;
    try {
        isRead = ReadFile(hDevice, lpbDataBuf, readBufSize, &NumberOfBytesRead, &overlappedRead);

        if(!isRead && GetLastError() == ERROR_IO_PENDING) {

            int dwRet = WaitForSingleObject(overlappedRead.hEvent, timeout);
            if (dwRet != WAIT_OBJECT_0) {

                cerr << "read waitForSingleObject error code: " << GetLastError() << endl;

                throw(0);
            }

            if (!GetOverlappedResult(hDevice, &overlappedRead, &NumberOfBytesTransferred, TRUE)) {

                cerr << "GetOverlappedResult error code: " << GetLastError() << endl;

                throw(0);
            }
            else {
                // Overlapped operation success is not equal to ReadFile operaion success.
                if (lpbDataBuf == NULL) {

                    cout << "receive nothing" << endl;

                    throw(0);
                }
            }
        }
    }
    catch (...) {

        CloseHandle(overlappedRead.hEvent);
        overlappedRead.hEvent = NULL;

        delete[] lpbDataBuf;

        return 0;
    }

    CloseHandle(overlappedRead.hEvent);
    overlappedRead.hEvent = NULL;

    for (int iter = 0; iter < readBufSize; ++iter)
        readDataBuffer[iter] = lpbDataBuf[iter];

    delete[] lpbDataBuf;

    return 1;
}

string Connector::takeout_val(int&& pos, bool&& isChange)
{
    // print readfile value
    string val = "";

    // Type and hex casting
    if (isChange) {
        std::stringstream ss;
        for (int iter = 0; iter <= pos; ++iter) {
            int bufferVal = (int)readDataBuffer[iter];
            if (bufferVal < 0)
                ss << std::hex << -(bufferVal ^ 255) - 1;
            else
                ss << std::hex << bufferVal;

            val += ss.str();

            ss.str("");
            ss.clear();
        }
    }
    else {
        for (int iter = 0; iter <= pos; ++iter)
            val += readDataBuffer[iter];
    }

    readDataBuffer.clear();
    readDataBuffer.resize(max_buffer_size);

    return val;
}

string Connector::takeout_val(const int& pos, const bool& isChange)
{
    // print readfile value
    string val = "";

    // Type and hex casting
    if (isChange) {
        std::stringstream ss;
        for (int iter = 0; iter <= pos; ++iter) {
            int bufferVal = (int)readDataBuffer[iter];
            if (bufferVal < 0)
                ss << std::hex << -(bufferVal ^ 255) - 1;
            else
                ss << std::hex << bufferVal;

            val += ss.str();

            ss.str("");
            ss.clear();
        }
    }
    else {
        for (int iter = 0; iter <= pos; ++iter)
            val += readDataBuffer[iter];
    }

    readDataBuffer.clear();
    readDataBuffer.resize(max_buffer_size);

    return val;
}

void Connector::close()
{
    FlushFileBuffers(hDevice);

    CancelIo(hDevice);

    CloseHandle(hDevice);
}

///////////
/// usb ///
///////////
Usb::Usb()
{
    hDevice = NULL;

    timeout = 2000;

    max_buffer_size = 256;

    readDataBuffer = std::vector<char>(max_buffer_size, 0);
}

Usb::Usb(const int& time, const int& size)
{
    hDevice = NULL;

    timeout = time;

    max_buffer_size = size;

    readDataBuffer = std::vector<char>(max_buffer_size, 0);
}

Usb::Usb(int&& time, int&& size)
{
    hDevice = NULL;

    timeout = time;

    max_buffer_size = size;

    readDataBuffer = std::vector<char>(max_buffer_size, 0);
}

Usb::~Usb() noexcept
{

}

Usb::Usb(const Usb& copyUsb)
{
    hDevice = copyUsb.hDevice;

    timeout = copyUsb.timeout;

    max_buffer_size = copyUsb.max_buffer_size;

    readDataBuffer = std::vector<char>(max_buffer_size, 0);
    readDataBuffer = copyUsb.readDataBuffer;
}

void Usb::get_device()
{
    HKEY hKey, hValueKey, hConnectKey;
    int nSubKeyResult, nValueResult;
    DWORD dwValueIndex, dwValueNameLen, dwValueDataLen, dwValueDataType;
    DWORD dwSubKeyIndex, dwSubKeyNameLen;
    TCHAR* pszSubKeyName, * pszSubKey, szValueName[64];
    BYTE* pbValueData;
    TCHAR acSerialNumber[256];
    FILETIME filetime;
    DWORD dwConnectNumb = 64, dwDeviceInsLen;
    TCHAR* pszDeviceInstance;
    BYTE* pbCountData;

    ZeroMemory(m_aUsbRegHead, sizeof(USB_REG) * 20);

    // HKEY_LOCAL_MACHINE\SYSTEM\ControlSet001\Control\DeviceClasses\{28d78fad-5a12-11d1-ae5b-0000f803a8c2}
    // HKEY_LOCAL_MACHINE\SYSTEM\ControlSet002\Control\DeviceClasses\{28d78fad-5a12-11d1-ae5b-0000f803a8c2}
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Control\\DeviceClasses\\{28d78fad-5a12-11d1-ae5b-0000f803a8c2}"), 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return;

    #define TEMP_BUF_LEN 512
    pszSubKeyName = new TCHAR[TEMP_BUF_LEN];
    pszSubKey = new TCHAR[TEMP_BUF_LEN];
    pbValueData = new BYTE[TEMP_BUF_LEN];
    pszDeviceInstance = new TCHAR[TEMP_BUF_LEN];
    pbCountData = new BYTE[12];
    dwDeviceInsLen = TEMP_BUF_LEN;

    int nEnumNum = 0;
    dwSubKeyIndex = 0;
    do {
        dwSubKeyNameLen = TEMP_BUF_LEN;//sizeof(szSubKeyName);
        nSubKeyResult = RegEnumKeyEx(hKey, dwSubKeyIndex, pszSubKeyName, &dwSubKeyNameLen, NULL, NULL, NULL, &filetime);
        if (nSubKeyResult == ERROR_SUCCESS) {
            //===== Get Serial Number
            if (RegOpenKeyEx(hKey, pszSubKeyName, 0, KEY_READ, &hValueKey) == ERROR_SUCCESS) {
                dwValueIndex = 0;
                do {
                    dwValueNameLen = sizeof(szValueName);
                    dwValueDataLen = TEMP_BUF_LEN;//sizeof(bValueData);
                    nValueResult = RegEnumValue(hValueKey, dwValueIndex, szValueName, &dwValueNameLen, NULL, &dwValueDataType, pbValueData, &dwValueDataLen);
                    if (nValueResult == ERROR_SUCCESS) {
                        if (wcsstr(szValueName, _T("DeviceInstance")))
                            wcscpy_s(acSerialNumber, (TCHAR*)pbValueData);
                    }
                    dwValueIndex++;
                } while (nValueResult == ERROR_SUCCESS);
            }

            //===== Get Link
            BOOL bLinkOn = FALSE;
            swprintf_s(pszSubKey, TEMP_BUF_LEN, _T("%s\\#\\Control"), pszSubKeyName);
            {// for Win 7/8/10

                bLinkOn = 0;

                if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Services\\usbprint\\Enum"), 0, KEY_READ, &hConnectKey) != ERROR_SUCCESS)
                    //return 0;
                    break;

                if (RegQueryValueEx(hConnectKey, _T("Count"), NULL, NULL, pbCountData, &dwConnectNumb) != ERROR_SUCCESS) //get number of connected devices
                    //return 0;
                    break;

                //byte to int
                int nConnectNum = 0xFFFFFFFF;
                memcpy(&nConnectNum, pbCountData, 4);

                // Origin type is CString
                std::wstring index;

                swprintf_s(pszSubKey, TEMP_BUF_LEN, _T("%s\\#\\Device Parameters"), pszSubKeyName);

                if (RegOpenKeyEx(hKey, pszSubKey, 0, KEY_READ, &hValueKey) == ERROR_SUCCESS) {

                    wcscpy_s(m_aUsbRegHead[nEnumNum].acDeviceInstance, pszSubKeyName);
                    m_aUsbRegHead[nEnumNum].acDeviceInstance[0] = '\\';
                    m_aUsbRegHead[nEnumNum].acDeviceInstance[1] = '\\';
                    m_aUsbRegHead[nEnumNum].acDeviceInstance[3] = '\\';
                    dwValueIndex = 0;

                    do {
                        dwValueNameLen = sizeof(szValueName);
                        dwValueDataLen = TEMP_BUF_LEN;//sizeof(bValueData);
                        nValueResult = RegEnumValue(hValueKey, dwValueIndex, szValueName, &dwValueNameLen, NULL, &dwValueDataType, pbValueData, &dwValueDataLen);

                        if (nValueResult == ERROR_SUCCESS) {

                            for (int i = 0; i < nConnectNum; i++) {

                                dwDeviceInsLen = TEMP_BUF_LEN; //重置

                                index = std::to_wstring(i);

                                //int x = RegQueryValueEx(hConnectKey, index.c_str(), 0, NULL, (LPBYTE)pszDeviceInstance, &dwDeviceInsLen);
                                RegQueryValueEx(hConnectKey, index.c_str(), 0, NULL, (LPBYTE)pszDeviceInstance, &dwDeviceInsLen);

                                for (int i = 0; i < _MAX_PATH && pszDeviceInstance[i]; i++) {

                                    if (pszDeviceInstance[i] == '\\')
                                        pszDeviceInstance[i] = '#';
                                }

                                if (wcsstr(m_aUsbRegHead[nEnumNum].acDeviceInstance, pszDeviceInstance)) {

                                    if (wcsstr(szValueName, _T("Port Description"))) {
                                        wcscpy_s(m_aUsbRegHead[nEnumNum].acPortDescription, (TCHAR*)pbValueData);
                                    }

                                    if (wcsstr(szValueName, _T("Port Number"))) {
                                        memcpy(&m_aUsbRegHead[nEnumNum].nPortNumber, pbValueData, 4);
                                        bLinkOn = 1;
                                    }
                                    wcscpy_s(m_aUsbRegHead[nEnumNum].acSerialNumber, wcsrchr(acSerialNumber, TCHAR('\\')) + 1);
                                }
                            }
                        }
                        dwValueIndex++;

                    } while (nValueResult == ERROR_SUCCESS);

                    //===== Only support SBARCO printer
                    TCHAR acPortDescriptionTemp[64];
                    wcscpy_s(acPortDescriptionTemp, m_aUsbRegHead[nEnumNum].acPortDescription);
                    _wcsupr_s(acPortDescriptionTemp, 64);
                    //if(wcsstr(acPortDescriptionTemp, "SBARCO")) // 工廠測試程式不用過濾

                    if (bLinkOn)
                        nEnumNum++;
                }
            }
        }
        dwSubKeyIndex++;
    } while (nSubKeyResult == ERROR_SUCCESS);

    if (pszDeviceInstance)
        delete[] pszDeviceInstance;

    if (pbCountData)
        delete[] pbCountData;

    if (pszSubKeyName)
        delete[] pszSubKeyName;

    if (pszSubKey)
        delete[] pszSubKey;

    if (pbValueData)
        delete[] pbValueData;

    return;
}

void Usb::reset()
{
	GUID GUID_DEVCLASS_PORTS = {0xA5DCBF10, 0x6530, 0x11D2, {0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED}};

	HDEVINFO hDevInfo;
	SP_DEVINFO_DATA DeviceInfoData;

	// get device class information handle
	hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_PORTS, 0, 0, DIGCF_PRESENT);
	if (hDevInfo == INVALID_HANDLE_VALUE)
		cout << "SetupDiGetClassDevs error: " << GetLastError() << endl;

	for (int i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData); i++) {
		DWORD DataT;
		char friendly_name[2046] = { 0 };
		DWORD buffersize = 2046;
		DWORD req_bufsize = 0;

		// get device description information
		if (!SetupDiGetDeviceRegistryPropertyA(hDevInfo, &DeviceInfoData, SPDRP_CLASSGUID, &DataT, (PBYTE)friendly_name, buffersize, &req_bufsize)) {
			cout << ("SetupDiGetDeviceRegistryPropertyA error: ") << GetLastError() << endl;
		}
	}

	SetupDiRestartDevices(hDevInfo, &DeviceInfoData);
}

