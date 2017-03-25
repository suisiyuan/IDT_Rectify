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
    if (socket->state() == QAbstractSocket::UnconnectedState)
    {
        socket->connectToHost("192.168.1.7", 20108);
        if (!socket->waitForConnected(1000))
        {
            QMessageBox::warning(this, tr("Connect failed"), tr("Connnot connect to encoder(192.168.1.7)"));
            return;
        }
        else
        {
            ui->connectButton->setText(tr("Disconnect"));
            ui->startButton->setEnabled(true);

        }
    }
    else if (socket->state() == QAbstractSocket::ConnectedState)
    {
        socket->close();
        ui->connectButton->setText(tr("Connect"));
        ui->startButton->setEnabled(false);
        ui->rectifyButton->setEnabled(false);
        ui->currentEdit->clear();
        ui->doubleSpinBox->setValue(0.0);
    }


}

void Widget::handleStartClicked()
{
    if (!started)
    {
        quint8 startMsg[] = { 0x7E, 0x00, 0x01, 0xD1, 0xF1 };
        socket->write((char *)startMsg, sizeof(startMsg));
        QObject::connect(socket, SIGNAL(readyRead()), this, SLOT(handleReceiveData));

        ui->startButton->setText(tr("Stop"));
        ui->rectifyButton->setEnabled(false);
        started = !started;
    }
    else
    {
        quint8 stopMsg[] = { 0x7E, 0x00, 0x02, 0xB2, 0xC1 };
        socket->write((char *)stopMsg, sizeof(stopMsg));
        QObject::disconnect(socket, SIGNAL(readyRead()), this, SLOT(handleReceiveData));

        ui->startButton->setText(tr("Start"));
        ui->rectifyButton->setEnabled(true);
        started = !started;
    }


}

void Widget::handleRectifyClicked()
{
    if (ui->currentEdit->text().isEmpty())
    {
        QMessageBox::warning(this, tr("Rectify failed"), tr("Please click start first!"));
        return;
    }
    else if (ui->currentEdit->text().toInt() == 0)
    {
        QMessageBox::warning(this, tr("Rectify failed"), tr("Current height connot be 0!"));
        return;
    }
    else
    {
        QSettings setting("HKEY_CURRENT_USER\\SOFTWARE\\ylink\\IDT\\Settings", QSettings::NativeFormat);
        double ratio = ui->doubleSpinBox->value() / ui->currentEdit->text().toInt();
        setting.setValue("Declination", QVariant(ratio));
        QMessageBox::information(this, tr("Write succeeded"), tr("Write to registry successfully!"));
    }
}


void Widget::handleReceiveData()
{
    QByteArray data = socket->readAll();
    switch ((quint8)data.at(2))
    {
        case 0x05:
        {
            ui->currentEdit->setText(QString::number(qFromBigEndian<qint32>((void *)data.mid(3, 4).data())));
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
        QMessageBox::warning(this, tr("Connect failed"), tr("Connnot connect to probe (192.168.1.10)"));
    }
    else
    {
        quint8 msg[] = { 0x7E, 0x04, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
        memcpy(msg+3, (uchar *)&value, sizeof(value));
        CRC crc = crcSlow(msg+2, msg[1]+1);
        memcpy(msg+3+msg[1], (uchar *)&crc, sizeof(crc));

        QByteArray array((char *)msg, sizeof(msg));
        qDebug() << array.toHex().toUpper();

        s->write((char *)msg, sizeof(msg));
    }

    s->close();
}



// turn on
void Widget::on_autoButton_clicked()
{
    QTcpSocket *s = new QTcpSocket();
    s->connectToHost("192.168.1.10", 20108);

    if (!s->waitForConnected(1000))
    {
        QMessageBox::warning(this, tr("Connect failed"), tr("Connnot connect to probe (192.168.1.10)"));
    }
    else
    {
        quint8 startMsg[] = { 0x7E, 0x01, 0x09, 0x01, 0xB6, 0xB7 };
        s->write((char *)startMsg, sizeof(startMsg));
    }

    s->close();
}

// turn off
void Widget::on_manualButton_clicked()
{
    QTcpSocket *s = new QTcpSocket();
    s->connectToHost("192.168.1.10", 20108);

    if (!s->waitForConnected(1000))
    {
        QMessageBox::warning(this, tr("Connect failed"), tr("Connnot connect to probe (192.168.1.10)"));
    }
    else
    {
        quint8 startMsg[] = { 0x7E, 0x01, 0x09, 0x00, 0x97, 0xA7 };
        s->write((char *)startMsg, sizeof(startMsg));
    }

    s->close();
}



void Widget::on_getButton_clicked()
{
    QTcpSocket *s = new QTcpSocket();
    s->connectToHost("192.168.1.10", 20108);

    if (!s->waitForConnected(1000))
    {
        QMessageBox::warning(this, tr("Connect failed"), tr("Connnot connect to probe (192.168.1.10)"));
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

//                uchar temp[100] = {0};
//                memcpy(temp, data.data()+2, length+1);
//                CRC crc = crcSlow(temp, length+1);
//                QByteArray array((char *)&crc, 2);

//                qDebug() << data.count();
//                qDebug() << data.toHex().toUpper();
//                qDebug() << major << minor << patch;
//                qDebug() << sha1;
//                qDebug() << array.toHex().toUpper();

                ui->versionEdit->setText(QString::number(major) + "." + QString::number(minor) + "." + QString::number(patch));
                ui->sha1Edit->setText(sha1);

                break;
            }
            default:
                break;
            }
            s->close();
        });
    }



}
