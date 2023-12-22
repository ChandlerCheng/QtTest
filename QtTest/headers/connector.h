#ifndef CONNECTOR_H
#define CONNECTOR_H

// QT
#include <QObject>

// C++
#include <string>
#include <vector>
#include <any>
#include <type_traits>

// windows
#include <windows.h>
#include <winBase.h>
#include <dbt.h>

//////////////
/// struct ///
//////////////
struct USB_REG {
    TCHAR acDeviceInstance[128];
    TCHAR acPortDescription[64];
    TCHAR acSerialNumber[64];
    int  nPortNumber;
    bool isLinked = false;
};

////////////
/// base ///
////////////
class Connector
{
public:
    HANDLE hDevice;

    int timeout;

    int max_buffer_size;
    // store the data from read function
    std::vector<char> readDataBuffer;

public:
    /*
     * constructor w/ default
    */
    Connector();
    /*
     * destructor
     * - should be virtual
     * - it will be crash if destructor has exception
    */
    virtual ~Connector() noexcept;
    /*
     * pure virtual
     * - let "class Connector" cannot construct in dynamic allocation
     * - derive class should instantiate pure virtual function
    */ 
    virtual void get_device() = 0;
    /*
     * for serial port init
     * - lpFileName: serial number of type int
    */
    virtual int init(int lpFileName);
    /*
     * for usb port init
     * - lpFileName: usb port path of type TCHAR
    */
    virtual int init(TCHAR* lpFileName);
    /*
     * for any type init
     * - lpFileName: any type port of type any
     * - isOverlapped: create with overlapped or not
    */
    virtual int init(const std::any& lpFileName, bool isOverlapped = false);
    /*
     * send command to device
     * - command: command of unsigned char
    */
    virtual int write(QByteArray command);
    /*
     * receive data from device
    */
    virtual int read();
    /*
     * takeout val from readDataBuffer
     * - pos: how many data you want to return
     * - isChange: change data to hex or keep origin
    */
    virtual std::string takeout_val(int&& pos, bool&& isChange = false);
    virtual std::string takeout_val(const int& pos, const bool& isChange);
    /*
     * close the device of handle
    */
    virtual void close();
};

///////////
/// usb ///
///////////
class Usb : public Connector
{
public:
    USB_REG m_aUsbRegHead[20];

public:
    Usb();
    /*
     * constructor w/ reference
     * - time: timeout
     * - size: max_buffer_size
    */
    Usb(const int& time, const int& size);
    /*
     * constructor w/ rvalue
     * - time: timeout
     * - size: max_buffer_size
     * - use forward function insure to pass by rvalue, and be careful "reference collapse"
    */
    Usb(int&& time, int&& size);

    virtual ~Usb() noexcept;
    /*
     * copy constructor
    */
    Usb(const Usb& copyUsb);
    /*
     * get_device in constructor is pure virtual function,
     * so it have to instantiated when you inherit
    */
    virtual void get_device();
    /*
     * reset usb pipe
    */
    void reset();
};

///////////
/// com ///
///////////
class Serial : public Connector
{
public:
    /*
     * store all serial numbers in use
    */
    std::vector<std::string> portNum;

public:
    Serial();
    Serial(const int& time, const int& size);
    Serial(int&& time, int&& size);

    virtual ~Serial() noexcept;

    Serial(const Serial& copyCom);

    virtual void get_device();

    virtual int write(const unsigned char* command);

    virtual int read();

    /*
     * control setting for a serial communications device
     * baudrate: serial port baudrate
    */
    int com_setting(const int& baudrate);
};


#endif // CONNECTOR_H
