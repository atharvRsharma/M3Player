#include "mainwindow.h"

#include <QApplication>



int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    MainWindow w;
    //w.setWindowState(Qt::WindowMaximized);
    w.show();

    if (argc > 1) {
        QString filePath = QString::fromLocal8Bit(argv[1]);
        w.openCommandLineArgs(filePath);
    }

    return a.exec();
}
