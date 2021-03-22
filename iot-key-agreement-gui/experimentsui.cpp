#include "experimentsui.h"
#include "ui_experimentsui.h"

#include <QMessageBox>
#include <QThread>
#include <iostream>

ExperimentsUI::ExperimentsUI(Trevor *trevor, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ExperimentsUI)
{
    ui->setupUi(this);
    ui->widget_plot->xAxis->setLabel("Number of users");
    ui->widget_plot->yAxis->setLabel("Time (secs)");
    ui->widget_plot->addGraph();
    ui->widget_plot->graph(0)->setPen(QPen(Qt::blue));
    ui->widget_plot->graph(0)->setLineStyle(QCPGraph::LineStyle::lsLine);
    ui->widget_plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    if(!QDir("Data").exists()){
        QDir().mkdir("Data");
    }

    host = ui->lineEdit_host->text();
    port = ui->lineEdit_port->text().toInt();
    username = ui->lineEdit_username->text();
    password = ui->lineEdit_password->text();
    this->trevor = trevor;
    this->trevor->setHost(host);
    this->trevor->setPort(port);
    this->trevor->setUsername(username);
    this->trevor->setPassword(password);
    QObject::connect(trevor, &Trevor::energyConsumption, this, &ExperimentsUI::on_consumption_measurement);
}

ExperimentsUI::~ExperimentsUI()
{
    for(int i = 0; i < devices.size(); i++){
        delete devices[i];
        devices[i] = nullptr;
    }
    devices.clear();
    delete ui;
}

void ExperimentsUI::on_consumption_measurement(size_t device_id, size_t n_key, double consumption){
    std::cout <<"Energy consumption received: " << device_id << " " << n_key << " " << consumption << std::endl;
    QFile file("Data/energy_consumption_it_"+QString::number(it)+".csv");
    QTextStream writter(&file);

    if(energy_data_open && file.open(QIODevice::WriteOnly | QIODevice::Append)){
        writter << QString::number(device_id) << "," << n_key << "," << QString::number(consumption) << "\n";
    }else if(file.open(QIODevice::WriteOnly)){
        writter << "Device ID,Time, Consumption\n";
        writter << QString::number(device_id) << "," << n_key << "," << QString::number(consumption) << "\n";
        energy_data_open = true;
    }

    file.close();
}

void ExperimentsUI::run(){
    if(!running){
        return;
    }
    double prob = ui->prob_selector->value();
    QString distribution = ui->comboBox_distribution->currentText();
    size_t iterations = ui->spinBox_iterations->text().toUInt();
    size_t seed = ui->lineEdit_seed->text().toUInt();

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
                QObject::connect(devices.last(), &Device::energyConsumption, this, &ExperimentsUI::on_consumption_measurement);
                timer->start(200);
           }else if(it < iterations){
               on_pushButton_2_clicked();
               energy_data_open = false;
               it++;
           }else{
               running = false;
               it = 0;
               emit stopped();
               QObject::disconnect(timer, &QTimer::timeout, 0, 0);
           }
        });
        timer->start(100);
    }
}

void ExperimentsUI::on_pushButton_runExperiment_clicked()
{
    if(ui->lineEdit_host->text().isEmpty() || ui->lineEdit_port->text().isEmpty()){
        QMessageBox msgbox;
        msgbox.setIcon(QMessageBox::Warning);
        msgbox.setWindowTitle("IOT Key Agreement - Experiments");
        msgbox.addButton(QMessageBox::Ok);
        msgbox.setText("Warning: you need to provide a host and a port to connect.");
        msgbox.exec();
        return;
    }
    if(ui->lineEdit_ndevices->text().isEmpty()){
        QMessageBox msgbox;
        msgbox.setIcon(QMessageBox::Warning);
        msgbox.setWindowTitle("IOT Key Agreement - Experiments");
        msgbox.addButton(QMessageBox::Ok);
        msgbox.setText("Warning: you need to provide the number of devices used in the experiment.");
        msgbox.exec();
        return;
    }

    n_devices = ui->lineEdit_ndevices->text().toInt();
    host = ui->lineEdit_host->text();
    port = ui->lineEdit_port->text().toInt();
    username = ui->lineEdit_username->text();
    password = ui->lineEdit_password->text();
    running= true;
    trevor->connectToHost();
}

void ExperimentsUI::receiveComputationTime(double time, int n_users)
{
}

void ExperimentsUI::on_lineEdit_plot_title_textChanged(const QString &arg1)
{
    if (!plot_title.isNull()){
        plot_title = QString();
        ui->widget_plot->plotLayout()->removeAt(ui->widget_plot->plotLayout()->rowColToIndex(0, 0));
        ui->widget_plot->plotLayout()->simplify();
     }

    if (!arg1.isNull()){
        plot_title = arg1;
        ui->widget_plot->plotLayout()->insertRow(0);
        ui->widget_plot->plotLayout()->addElement(0, 0, new QCPTextElement(ui->widget_plot, plot_title));
    }
    ui->widget_plot->replot();
}

void ExperimentsUI::on_pushButton_load_data_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open Plot"), "Data/", tr("Plot Files (*.plt)"));
    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)){
        if(file.isOpen()){
            this->n_users.clear();
            this->time.clear();
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine();
                QStringList data = line.split("\t");
                this->n_users.append(data[0].toInt());
                this->time.append(data[1].toDouble());
            }
            file.close();
        }
    }
    ui->widget_plot->graph(0)->addData(this->n_users, this->time);
    ui->widget_plot->graph(0)->rescaleAxes(true);
    ui->widget_plot->replot();
}

void ExperimentsUI::on_lineEdit_x_title_textChanged(const QString &arg1)
{
    ui->widget_plot->xAxis->setLabel(arg1);
    ui->widget_plot->replot();
}

void ExperimentsUI::on_lineEdit_y_title_textChanged(const QString &arg1)
{
    ui->widget_plot->yAxis->setLabel(arg1);
    ui->widget_plot->replot();
}

void ExperimentsUI::on_comboBox_time_measurement_activated(const QString &arg1)
{
    emit measurementTypeChanged(arg1);
}

void ExperimentsUI::on_pushButton_2_clicked()
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

void ExperimentsUI::on_pushButton_clear_plot_clicked()
{
    n_users.clear();
    time.clear();
    ui->widget_plot->graph()->setData(n_users, time);
    ui->widget_plot->replot();
}
