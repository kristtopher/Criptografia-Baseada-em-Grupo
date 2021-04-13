#ifndef EXPERIMENTSUI_H
#define EXPERIMENTSUI_H

#include <qt5/QtCore/qthread.h>
#include <qt5/QtCore/QObject>
#include <qt5/QtCore/QString>
#include <qt5/QtCore/QTimer>
#include "trevor.h"
#include "device.h"


class Experiments : public QObject
{
    Q_OBJECT

public:
    explicit Experiments(QString& host, quint16 port, QString& username, QString& password);
    bool isRunning() { return this->running; }
    void setRunning() {this->running = true;}
    Trevor *trevor;

    ~Experiments();

private slots:
    void on_pushButton_runExperiment_clicked();

    void on_lineEdit_plot_title_textChanged(const QString &arg1);

    void on_pushButton_load_data_clicked();

    void on_lineEdit_x_title_textChanged(const QString &arg1);

    void on_lineEdit_y_title_textChanged(const QString &arg1);

    void on_comboBox_time_measurement_activated(const QString &arg1);

    void on_pushButton_2_clicked();

    void on_pushButton_clear_plot_clicked();

    void on_consumption_measurement(size_t device_id, double time, double consumption);
public slots:

    void receiveComputationTime(double time, int n_users);
     void run();

signals:
    void measurementTypeChanged(const QString &measurement_type);
    void iteration(int it);
    void stopped();
    void finished();
    void prepared();

private:
    QString plot_title;
    size_t n_devices, n_finished = 0;
    QVector<Device *> devices;
    QString host, username, password;
    quint16 port;
    QTimer *timer;
    QVector<double> time;
    QVector<double> n_users;
    bool running = false, energy_data_open = false;
    size_t it = 0;
    int total_comp_time = 0;
};

#endif // EXPERIMENTSUI_H
