//
// Created by mateus on 13/04/2021.
//
#include <iostream>
#include <QDebug>
#include "MyApp.h"

int main(int argc, char* argv[]){
    QString host = "127.0.0.1";
    quint16 port = 1883;
    QString username = "", password ="";
    MyApp app(argc, argv, host, port, username, password);

    app.runApp();
    qDebug() << "End";

    return MyApp::exec();
}
