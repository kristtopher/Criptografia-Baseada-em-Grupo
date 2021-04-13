#include "experiments.h"

#include <QThread>
#include <iostream>
#include <QDir>
#include <random>

Experiments::Experiments(QString& host, quint16 port, QString& username, QString& password) {
    if(!QDir("Data").exists()){
        QDir().mkdir("Data");
    }

    this->host = host;
    this->port = port;
    this->username = username;
    this->password = password;
    this->trevor = new Trevor(host, port, username, password);

    this->trevor->connectToHost();
    QObject::connect(trevor, &Trevor::energyConsumption, this, &Experiments::on_consumption_measurement);
    QObject::connect(trevor, &Trevor::serverConnected, this, [this](){
       std::cout << "Broker connected." << std::endl;
       emit this->prepared();
    });
    QObject::connect(trevor, &Trevor::sessionKeyComputed, this, [](const QString user, const QString session_key){
        qDebug() << "Key type: " << user << " Session key: " << session_key;
    });
}

Experiments::~Experiments()
{
    for(int i = 0; i < devices.size(); i++){
        delete devices[i];
        devices[i] = nullptr;
    }
    delete trevor;
    devices.clear();
}

void Experiments::on_consumption_measurement(size_t device_id, double time, double consumption){
    QFile file("Data/energy_consumption_it_"+QString::number(it)+".csv");
    QTextStream writter(&file);

    if(energy_data_open && file.open(QIODevice::WriteOnly | QIODevice::Append)){
        writter << QString::number(device_id) << "," << QString::number(time) << "," << QString::number(consumption) << "\n";
    }else if(file.open(QIODevice::WriteOnly)){
        writter << "Device ID,Time, Consumption\n";
        writter << QString::number(device_id) << "," << QString::number(time) << "," << QString::number(consumption) << "\n";
        energy_data_open = true;
    }

    file.close();
}

void Experiments::run(){
    if(!running){
        return;
    }
    double prob = 0.8;
    QString distribution = "Binomial";
    size_t iterations = 15;
    size_t seed = 42;
    n_devices = 10;

    if(distribution == "Binomial"){
        std::binomial_distribution<int> distribution(n_devices,prob);
        std::mt19937 g1(seed);

        std::vector<int> success(iterations);
        timer = new QTimer(this);
        for(size_t i = 0; i < iterations; i++){
            success[i] = distribution(g1);
        }
        QObject::connect(timer, &QTimer::timeout, this, [this, success, iterations](){

           if((devices.size() < success[it]) && (it < iterations)){
                emit iteration(it);
                devices.push_back(new Device(this->host, this->port, this->username, this->password));
                qDebug() << "Connecting device " << devices.size();
                QObject::connect(devices.last(), &Device::energyConsumption, this, &Experiments::on_consumption_measurement);
                timer->start(200);
           }else if(it < iterations){
               on_pushButton_2_clicked();
               energy_data_open = false;
               qDebug() << "\nEnd of iteration " << it;
               timer->start(200);
               it++;
           }else{
               running = false;
               it = 0;
               emit stopped();

               QObject::disconnect(timer, &QTimer::timeout, 0, 0);
               emit finished();
           }
        });
        timer->start(100);
    }
}

void Experiments::on_pushButton_runExperiment_clicked()
{
    n_devices = 10;

    running= true;
    trevor->connectToHost();
}

void Experiments::receiveComputationTime(double time, int n_users)
{
}

void Experiments::on_lineEdit_plot_title_textChanged(const QString &arg1)
{

}

void Experiments::on_pushButton_load_data_clicked()
{

}

void Experiments::on_lineEdit_x_title_textChanged(const QString &arg1)
{

}

void Experiments::on_lineEdit_y_title_textChanged(const QString &arg1)
{
}

void Experiments::on_comboBox_time_measurement_activated(const QString &arg1)
{
    emit measurementTypeChanged(arg1);
}

void Experiments::on_pushButton_2_clicked()
{
    for(int i = 0; i < devices.size(); i++){
        QObject::disconnect(devices[i], &Device::energyConsumption, 0, 0);
        delete devices[i];
        devices[i] = nullptr;
    }
    devices.clear();
    trevor->dropUsers();
    n_devices = 0;
}

void Experiments::on_pushButton_clear_plot_clicked()
{
    n_users.clear();
    time.clear();

}
