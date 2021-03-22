#ifndef EXPERIMENTSUI_H
#define EXPERIMENTSUI_H

#include <QWidget>
#include <QObject>

#include "trevor.h"
#include "device.h"

namespace Ui {
class ExperimentsUI;
}

class ExperimentsUI : public QWidget
{
    Q_OBJECT

public:
    explicit ExperimentsUI(Trevor *trevor, QWidget *parent = nullptr);
    bool isRunning() { return this->running; }

    ~ExperimentsUI();

private slots:
    void on_pushButton_runExperiment_clicked();

    void on_lineEdit_plot_title_textChanged(const QString &arg1);

    void on_pushButton_load_data_clicked();

    void on_lineEdit_x_title_textChanged(const QString &arg1);

    void on_lineEdit_y_title_textChanged(const QString &arg1);

    void on_comboBox_time_measurement_activated(const QString &arg1);

    void on_pushButton_2_clicked();

    void on_pushButton_clear_plot_clicked();

    void on_consumption_measurement(size_t device_id, size_t n_key, double consumption);
public slots:

    void receiveComputationTime(double time, int n_users);
     void run();

signals:
    void measurementTypeChanged(const QString &measurement_type);
    void iteration(int it);
    void stopped();
private:
    Ui::ExperimentsUI *ui;
    Trevor *trevor;
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
