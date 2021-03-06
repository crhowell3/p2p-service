#include <QtCore>
#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDebug>
#include <QMenuBar>
#include <QMessageBox>
#include <QObject>
#include <QScreen>
#include <QThread>
#include <QWidget>
#include <iostream>
#include "ServerThread.h"
#include "ui_ServerWindow.h"

Ui::ServerWindow ui;

void updateUi(const QString& message)
{
    ui.serverStatus->append(message);
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    std::unique_ptr<QWidget> widget = std::make_unique<QWidget>();
    ui.setupUi(widget.get());

    // Set serverStop to disable initially
    ui.serverStop->setEnabled(false);

    // Setup server thread
    std::unique_ptr<QThread> server_thread = std::make_unique<QThread>();

    // Setup server object
    ServerThread server;

    // Move the server process to a QThread
    server.moveToThread(server_thread.get());

    // Connect signals and slots
    QObject::connect(server_thread.get(), &QThread::started, &server, &ServerThread::serverMain);
    QObject::connect(&server, &ServerThread::finished, server_thread.get(), &QThread::quit);
    QObject::connect(&server, &ServerThread::finished, &server, &ServerThread::deleteLater);
    QObject::connect(server_thread.get(), &QThread::finished, server_thread.get(), &QThread::deleteLater);
    QObject::connect(&server, &ServerThread::serverUpdated, updateUi);
    QObject::connect(ui.serverStart, &QPushButton::clicked, [&server_thread]() {
        server_thread->start();
        QApplication::processEvents();
        ui.serverStart->setEnabled(false);
        ui.serverStop->setEnabled(true);
    });
    QObject::connect(ui.serverStop, &QPushButton::clicked, [&server, &server_thread]() {
        ui.serverStatus->append("Shutting down the server...");
        server.BeginThreadAbortion();
        if (server_thread->wait(2000))
        {
            server_thread->terminate();
            server_thread->wait();
        }
        ui.serverStatus->append("Server shut down.");
        ui.serverStart->setEnabled(true);
        ui.serverStop->setEnabled(false);
    });

    widget->show();
    return app.exec();
}