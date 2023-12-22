#ifndef USBLISTEN_H
#define USBLISTEN_H

#include <QWidget>
#include <QAbstractNativeEventFilter>

class usbListen : public QWidget, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    usbListen();

    void emitPlugIn();

protected:
    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override;

signals:
    void devicePlugIn();

    void devicePlugOut();
};

#endif // USBLISTEN_H
