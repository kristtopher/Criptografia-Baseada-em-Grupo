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
    explicit ExperimentsUI(QWidget *parent = nullptr);
    ~ExperimentsUI();

private slots:
    void on_pushButton_runExperiment_clicked();

    void on_pushButton_save_pdf_clicked();

    void on_pushButton_clicked();

    void on_lineEdit_plot_title_textChanged(const QString &arg1);

    void on_pushButton_load_data_clicked();

    void on_lineEdit_x_title_textChanged(const QString &arg1);

    void on_lineEdit_y_title_textChanged(const QString &arg1);

    void on_comboBox_time_measurement_activated(const QString &arg1);

    void on_pushButton_2_clicked();

    void on_pushButton_clear_plot_clicked();

public slots:

    void receiveComputationTime(double time, int n_users);

signals:
    void measurementTypeChanged(const QString &measurement_type);

private:
    Ui::ExperimentsUI *ui;
    QString plot_title;
    size_t n_devices, n_finished = 0;
    QVector<Device *> devices;
    QString host, username, password;
    quint16 port;
    QTimer *timer;
    QVector<double> time;
    QVector<double> n_users;
    int total_comp_time = 0;
};

#endif // EXPERIMENTSUI_H
