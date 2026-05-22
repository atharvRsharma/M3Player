#include "mainwindow.h"

#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>


int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    const QString serverName = "M3Player";
    QList<QString> paths;
    if (argc > 1) {
        for (size_t i{1} ; i < argc ; ++i) {
            paths.append(QString::fromLocal8Bit(argv[i]));
        }
    }

    QLocalSocket socket;
    socket.connectToServer(serverName);
    if (socket.waitForConnected(500)) {
        socket.write(paths.join("\n").toUtf8());
        socket.flush();
        socket.waitForBytesWritten(500);
        return 0;
    }

    QLocalServer server;
    QLocalServer::removeServer(serverName);
    server.listen(serverName);

    MainWindow w;
    w.setWindowState(Qt::WindowMaximized);
    w.show();

    for (auto &p : paths) {
        w.openCommandLineArgs(p);
    }

    QObject::connect(&server, &QLocalServer::newConnection, &server, [&] {
        QLocalSocket *client = server.nextPendingConnection();
        client->waitForReadyRead(500);
        QString incoming = QString::fromUtf8(client->readAll());
        if (!incoming.isEmpty()) {
            for (auto &p : incoming.split("\n", Qt::SkipEmptyParts)) {
                w.openCommandLineArgs(p);
            }
        }
        w.raise();
        w.activateWindow();
        client->deleteLater();
    });


    return a.exec();
}
