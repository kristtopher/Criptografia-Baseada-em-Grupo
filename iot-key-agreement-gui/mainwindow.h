#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStringListModel>

#include "paramsgui.h"
#include "experimentsui.h"
#include "trevor.h"
#include "device.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_connect_clicked();

    void addUserToListView(const QString);

    void addSessionKeyToView(const QString user, const QString session_key);

    void addLogMessageToView(const QString&);

    void on_pushButton_parameters_clicked();

    void on_pushButton_clear_clicked();

    void on_pushButton_clicked();

    void on_pushButton_new_device_clicked();

    void on_pushButton_remove_device_clicked();

private:
    Trevor *trevor;
    QString host, port, username, password;
    QStringList users_list;
    QStringListModel *users_model;
    QStringList sesskey_list;
    QStringListModel *sesskey_model;
    QStringList log_list;
    QStringListModel *log_model;
    QVector<Device*> devices;
    Ui::MainWindow *ui;
    ExperimentsUI *experiments;
    ParamsGUI *params_ui;
};

#endif // MAINWINDOW_H
