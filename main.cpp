#include "widget.h"
#include <QApplication>
#include <QTranslator>
#include <QLocale>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCoreApplication::setOrganizationName("ylink");
    QCoreApplication::setApplicationName("IDT");


    QString locale = QLocale::system().name();
    QTranslator translator;
    translator.load(QString(":/IDT_Rectify_") + locale);
    a.installTranslator(&translator);

    Widget w;
    w.show();

    return a.exec();
}
