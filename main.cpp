#include "mainwindow.h"

#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>


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
        socket.disconnectFromServer();
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
        auto *buffer = new QByteArray();
        auto *timer = new QTimer(client);
        timer->setSingleShot(true);
        timer->setInterval(50);

        QObject::connect(client, &QLocalSocket::readyRead, client, [client, buffer, timer]() {
            buffer->append(client->readAll());
            timer->start();
        });

        QObject::connect(timer, &QTimer::timeout, client, [&w, client, buffer]() {
            QString incoming = QString::fromUtf8(*buffer);
            delete buffer;
            if (!incoming.isEmpty())
                for (auto &p : incoming.split("\n", Qt::SkipEmptyParts))
                    w.openCommandLineArgs(p);
            w.raise();
            w.activateWindow();
            client->deleteLater();
        });

        QObject::connect(client, &QLocalSocket::disconnected, client, [timer]() {
            if (!timer->isActive())
                timer->start();
        });
    });


    return a.exec();
}
