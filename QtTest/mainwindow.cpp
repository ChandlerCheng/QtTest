
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <iostream>
#include <exception>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <type_traits>
#include <algorithm>
#include <thread>
#include <future>
#include <chrono>
#include <ratio>
#include "Windows.h"
#include "Dbt.h"
#include <QTime>
#include <QDebug>
#include <QMessageBox>
#include <QThread>
#include <QThreadPool>
#include <QFile>
#include <QResizeEvent>
#include <QListView>
#include <QTextCodec>
#include <QLabel>
#include <QDateTime>
#include <QRegularExpression>

using std::string;
using std::cout;
using std::endl;


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::usbInfoUpdate()
{
    usb_port->get_device();

    for(int DeviceNum=0;DeviceNum<20;DeviceNum++)
    {
        if(usb_port->m_aUsbRegHead[DeviceNum].acDeviceInstance==(TCHAR*)"")
            break;
        else
        {
            bool isInit = usb_port->init(usb_port->m_aUsbRegHead[DeviceNum].acDeviceInstance);
            if (isInit <= 0)
                return;
        }
    }
}

void MainWindow::on_btnTest1_clicked()
{
    ui->labTest->setText("123123");
    usbInfoUpdate();
}

