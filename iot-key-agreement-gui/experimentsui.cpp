#include "experimentsui.h"
#include "ui_experimentsui.h"

#include <QMessageBox>
#include <QThread>
#include <iostream>

ExperimentsUI::ExperimentsUI(QWidget *parent) :
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
    QString type_measurement = ui->comboBox_time_measurement->currentText();
    emit measurementTypeChanged(type_measurement);
}

ExperimentsUI::~ExperimentsUI()
{
    delete ui;
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

    timer = new QTimer(this);
    QObject::connect(timer, &QTimer::timeout, this, [this](){
       if(devices.size() < n_devices){
            devices.push_back(new Device(this->host, this->port, this->username, this->password));
            (*(devices.end()-1))->setN_cobaias(this->n_devices);
            timer->start(2000);
       }
    });
    timer->start(1000);
}

void ExperimentsUI::receiveComputationTime(double time, int n_users)
{
    if(this->time.size() == 0){
        this->time.push_back(0);
        this->n_users.push_back(0);
    }
    this->time.push_back(time);
    this->n_users.push_back(n_users);
    ui->widget_plot->graph(0)->addData(this->n_users, this->time);
    ui->widget_plot->graph(0)->rescaleAxes(true);
    ui->widget_plot->replot();
}

void ExperimentsUI::on_pushButton_save_pdf_clicked()
{
    QString pdf_name;
    if(ui->lineEdit_pdf_name->text().isEmpty()){
        pdf_name = plot_title + ".pdf";
    }else{
        pdf_name = ui->lineEdit_pdf_name->text() + ".pdf";
    }
    ui->widget_plot->savePdf(QString("Data/") + pdf_name);
}

void ExperimentsUI::on_pushButton_clicked()
{
    QString fname;
    if(ui->lineEdit_data_name->text().isEmpty()){
        fname = QString("Data/") + QString("data_") + QString::number(n_users.size()) + QString("devices.plt");
    }else{
        fname = QString("Data/") + ui->lineEdit_data_name->text() + ".plt";
    }
    QFile file(fname);
    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }

    QTextStream out(&file);

    for(int i = 0; i < n_users.size(); i++){
        out << n_users[i] << "\t" << time[i] << "\n";
    }

    file.close();

}

void ExperimentsUI::on_lineEdit_plot_title_textChanged(const QString &arg1)
{
    if (!plot_title.isNull()){
        plot_title = QString();
        ui->widget_plot->plotLayout()->removeAt(ui->widget_plot->plotLayout()->rowColToIndex(0, 0));
        ui->widget_plot->plotLayout()->simplify();
     }

      // Add a title if a non-null string was passed.
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
        delete devices[i];
        devices[i] = nullptr;
    }
    devices.clear();
    n_devices = 0;
}

void ExperimentsUI::on_pushButton_clear_plot_clicked()
{
    n_users.clear();
    time.clear();
    ui->widget_plot->graph()->setData(n_users, time);
    ui->widget_plot->replot();
}
