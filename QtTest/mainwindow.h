#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "headers/usblisten.h"
#include "headers/connector.h"

#include <memory>
#include <vector>
#include <string>

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {class MainWindow;}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnTest1_clicked();

private:
    Ui::MainWindow *ui;
    usbListen* usbListener;
    std::shared_ptr<Usb> usb_port;
    void usbInfoUpdate();
};
#endif // MAINWINDOW_H
