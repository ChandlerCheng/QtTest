#include "headers/usblisten.h"

#include <iostream>
#include <windows.h>
#include <dbt.h>
#include <memory>

using namespace std;

usbListen::usbListen()
{

}

// Start program for first time.
void usbListen::emitPlugIn()
{
    emit devicePlugIn();
}

bool usbListen::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
{
    MSG* msg = static_cast<MSG*>(message);

    unsigned int msgType = msg->message;
    if (msgType == WM_DEVICECHANGE) {
        if (msg->wParam == DBT_DEVICEARRIVAL)
            emit devicePlugIn();
        else if (msg->wParam == DBT_DEVICEREMOVECOMPLETE)
            emit devicePlugOut();
    }

    return QWidget::nativeEvent(eventType, message, result);
}
