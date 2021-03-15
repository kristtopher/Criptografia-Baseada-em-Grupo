#ifndef PARAMSGUI_H
#define PARAMSGUI_H

#include <boost/multiprecision/gmp.hpp>
#include <QMap>
#include <QWidget>

namespace Ui {
class ParamsGUI;
}

class ParamsGUI : public QWidget
{
    Q_OBJECT

public:
    explicit ParamsGUI(QWidget *parent = nullptr);
    ~ParamsGUI();

public slots:
    void receiveParameters(QMap<QString, boost::multiprecision::mpz_int>);

private:
    Ui::ParamsGUI *ui;
    QMap<QString, boost::multiprecision::mpz_int> parameters;
};

#endif // PARAMSGUI_H
