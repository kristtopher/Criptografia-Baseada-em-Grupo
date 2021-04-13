//
// Created by mateus on 13/04/2021.
//

#ifndef IOT_KEY_CORE_MYAPP_H
#define IOT_KEY_CORE_MYAPP_H
#include <QCoreApplication>
#include "experiments.h"

class MyApp: public QCoreApplication
{
    Q_OBJECT
private slots:
    void on_trevor_connected(){


    }
public:
    MyApp(int &argc, char* argv[], QString host, quint16 port, QString& username, QString& password): QCoreApplication(argc, argv),
                                                                                                      host(host), port(port), username(username), password(password)
    {
        this->experiment = new Experiments(host, port, username, password);
        connect(this->experiment, &Experiments::prepared, this, [this](){
            this->experiment->setRunning();
            this->experiment->run();
        });

    }

    void runApp(){
        QEventLoop loop;
        connect(this, &MyApp::finished, &loop, &QEventLoop::quit);
        loop.exec(); //exec
    }

    signals:
            void finished();
private:
    QString host, port, username, password;
    Trevor *trevor = nullptr;
    Experiments *experiment = nullptr;
};


#endif //IOT_KEY_CORE_MYAPP_H
