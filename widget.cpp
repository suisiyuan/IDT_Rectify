#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    socket(new QTcpSocket()),
    started(false),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    createConnections();
}

Widget::~Widget()
{
    delete ui;
}

void Widget::createConnections()
{
    QObject::connect(ui->connectButton, SIGNAL(clicked()), this, SLOT(handleConnectClicked()));
    QObject::connect(ui->startButton, SIGNAL(clicked()), this, SLOT(handleStartClicked()));
    QObject::connect(ui->rectifyButton, SIGNAL(clicked()), this, SLOT(handleRectifyClicked()));

    QObject::connect(socket, SIGNAL(readyRead()), this, SLOT(handleReceiveData()));
}



void Widget::handleConnectClicked()
{
    started = false;
    // 连接
    if (socket->state() == QAbstractSocket::UnconnectedState)
    {
        socket->connectToHost("192.168.1.7", 20108);
        if (!socket->waitForConnected(1000))
        {
            QMessageBox::warning(this, tr("Connect failed"), tr("Cannot connect to the encoder."));
            return;
        }
        else
        {
            ui->connectButton->setText(tr("Disconnect"));
            ui->startButton->setEnabled(true);

        }
    }
    // 断开
    else if (socket->state() == QAbstractSocket::ConnectedState)
    {
        socket->close();
        ui->connectButton->setText(tr("Connect"));
        ui->startButton->setEnabled(false);
        ui->rectifyButton->setEnabled(false);
        ui->currentSpinBox->setValue(0.0);
        ui->realSpinBox->setValue(0.0);
    }


}

void Widget::handleStartClicked()
{
    if (!started)
    {
        quint8 startMsg[] = { 0x7E, 0x00, 0x01, 0xD1, 0xF1 };
        socket->write((char *)startMsg, sizeof(startMsg));
//        QObject::connect(socket, SIGNAL(readyRead()), this, SLOT(handleReceiveData));

        ui->startButton->setText(tr("Stop"));
        ui->rectifyButton->setEnabled(false);
        started = !started;
    }
    else
    {
        quint8 stopMsg[] = { 0x7E, 0x00, 0x02, 0xB2, 0xC1 };
        socket->write((char *)stopMsg, sizeof(stopMsg));
//        QObject::disconnect(socket, SIGNAL(readyRead()), this, SLOT(handleReceiveData));

        ui->startButton->setText(tr("Start"));
        ui->rectifyButton->setEnabled(true);
        started = !started;
    }


}

void Widget::handleRectifyClicked()
{
//    if (ui->currentSpinBox->text().isEmpty())
//    {
//        QMessageBox::warning(this, tr("Rectify failed"), tr("Please click start first!"));
//        return;
//    }
//    else if (ui->realSpinBox->text().toInt() == 0)
//    {
//        QMessageBox::warning(this, tr("Rectify failed"), tr("Current height connot be 0!"));
//        return;
//    }
//    else
//    {
        QSettings setting;
        qreal currentHeight = ui->currentSpinBox->value();
        qreal realHeight = ui->realSpinBox->value();
        qreal ratio = realHeight / currentHeight;
        setting.setValue("Settings/Declination", ratio);
        QMessageBox::information(this, tr("Write succeeded"), tr("Write to registry successfully!"));
//    }
}


void Widget::handleReceiveData()
{
    QByteArray data = socket->readAll();
    switch ((quint8)data.at(2))
    {
        case 0x05:
        {
            qint32 pulseNum = qFromBigEndian<qint32>((void *)data.mid(3, 4).data());
            qreal height = pulseNum / 10000.0f;
            ui->currentSpinBox->setValue(height);
            break;
        }

        default:
            break;
    }
}

// Compass
void Widget::on_shiftButton_clicked()
{
    float value = 0.0;
    bool ok = false;

    if (ui->shiftLabel->text().isEmpty())
    {
        QMessageBox::warning(this, tr("Empty value"), tr("The value is empty."));
        return;
    }

    value = ui->shiftEdit->text().toFloat(&ok);
    if (!ok)
    {
        QMessageBox::warning(this, tr("Invalid value"), tr("The value is Invalid."));
        return;
    }

    QTcpSocket *s = new QTcpSocket();
    s->connectToHost("192.168.1.10", 20108);

    if (!s->waitForConnected(1000))
    {
        QMessageBox::warning(this, tr("Connect failed"), tr("Cannot connect to the probe."));
    }
    else
    {
        quint8 msg[] = { 0x7E, 0x04, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
        memcpy(msg+3, (uchar *)&value, sizeof(value));
        CRC crc = crcSlow(msg+2, msg[1]+1);
        memcpy(msg+3+msg[1], (uchar *)&crc, sizeof(crc));

//        QByteArray array((char *)msg, sizeof(msg));
//        qDebug() << array.toHex().toUpper();

        s->write((char *)msg, sizeof(msg));
    }

    s->close();
    s->deleteLater();
}


void Widget::on_getShiftButton_clicked()
{
    QTcpSocket *s = new QTcpSocket();
    s->connectToHost("192.168.1.10", 20108);

    if (!s->waitForConnected(1000))
    {
        QMessageBox::warning(this, tr("Connect failed"), tr("Cannot connect to the probe."));
    }
    else
    {
        quint8 msg[] = { 0x7E, 0x00, 0x0E, 0x00, 0x00 };
        CRC crc = crcSlow(msg+2, msg[1]+1);
        memcpy(msg+3+msg[1], (uchar *)&crc, sizeof(crc));
        s->write((char *)msg, sizeof(msg));
    }

    connect(s, &QTcpSocket::readyRead, [this, s]() {
        QByteArray data = s->readAll();
        switch ((quint8)data.at(2)) {
        case 0x8E:
        {
            qint32 length = (qint32)data.at(1);
            float value = qFromLittleEndian<float>((void *)(data.data()+3));
            ui->shiftEdit->setText(QString::number(value));
            break;
        }
        default:
            break;
        }
        s->close();
        s->deleteLater();
    });
}



// turn on
void Widget::on_autoButton_clicked()
{
    QTcpSocket *s = new QTcpSocket();
    s->connectToHost("192.168.1.10", 20108);

    if (!s->waitForConnected(1000))
    {
        QMessageBox::warning(this, tr("Connect failed"), tr("Cannot connect to the probe."));
    }
    else
    {
        quint8 startMsg[] = { 0x7E, 0x01, 0x09, 0x01, 0xB6, 0xB7 };
        s->write((char *)startMsg, sizeof(startMsg));
        QMessageBox::information(this, tr("Infomation"), tr("Already open."));
    }

    s->close();
    s->deleteLater();
}

// turn off
void Widget::on_manualButton_clicked()
{
    QTcpSocket *s = new QTcpSocket();
    s->connectToHost("192.168.1.10", 20108);

    if (!s->waitForConnected(1000))
    {
        QMessageBox::warning(this, tr("Connect failed"), tr("Cannot connect to the probe."));
    }
    else
    {
        quint8 startMsg[] = { 0x7E, 0x01, 0x09, 0x00, 0x97, 0xA7 };
        s->write((char *)startMsg, sizeof(startMsg));
        QMessageBox::information(this, tr("Infomation"), tr("Already closed."));
    }

    s->close();
    s->deleteLater();
}



void Widget::on_getButton_clicked()
{
    QTcpSocket *s = new QTcpSocket();
    s->connectToHost("192.168.1.10", 20108);

    if (!s->waitForConnected(1000))
    {
        QMessageBox::warning(this, tr("Connect failed"), tr("Cannot connect to the probe."));
    }
    else
    {
        quint8 msg[] = { 0x7E, 0x00, 0x08, 0x00, 0x00 };
        CRC crc = crcSlow(msg+2, msg[1]+1);
        memcpy(msg+3+msg[1], (uchar *)&crc, sizeof(crc));
        s->write((char *)msg, sizeof(msg));

        connect(s, &QTcpSocket::readyRead, [this, s]() {
            QByteArray data = s->readAll();
            switch ((quint8)data.at(2)) {
            case 0x88:
            {
                qint32 length = (qint32)data.at(1);
                quint32 version = qFromLittleEndian<qint32>((void *)(data.data()+3));

                quint8 major = (version & 0xFF0000) >> 16;
                quint8 minor = (version & 0xFF00) >> 8;
                quint8 patch = (version & 0xFF);

                QString sha1 = QString::fromLatin1(data.data()+3+sizeof(quint32));

                ui->versionEdit->setText(QString::number(major) + "." + QString::number(minor) + "." + QString::number(patch));
                ui->sha1Edit->setText(sha1);

                break;
            }
            default:
                break;
            }
            s->close();
            s->deleteLater();
        });
    }
}



void Widget::on_tabWidget_currentChanged(int index)
{
    qDebug() << index;
    if (index != 2)
    {
        ui->shiftEdit->clear();
    }
    if (index != 3)
    {
        ui->versionEdit->clear();
        ui->sha1Edit->clear();
    }
}
