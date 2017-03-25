#pragma once

#include <QWidget>

#include <QTcpSocket>
#include <QDebug>
#include <QMessageBox>
#include <QSettings>
#include <QtEndian>

#include "crc.h"

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();


private:
    Ui::Widget *ui;

    QTcpSocket *socket;
    bool started;

    void createConnections();

private slots:
    void handleConnectClicked();
    void handleStartClicked();
    void handleRectifyClicked();
    void handleReceiveData();

    void on_autoButton_clicked();
    void on_manualButton_clicked();
    void on_shiftButton_clicked();
    void on_getButton_clicked();
    void on_getShiftButton_clicked();
};

