#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    qDebug() << "heloo";
    QApplication a(argc, argv);
    MainWindow w;
    w.setWindowState(Qt::WindowMaximized);
    w.show();

    return a.exec();
}
