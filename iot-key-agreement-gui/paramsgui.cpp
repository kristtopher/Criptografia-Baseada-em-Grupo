#include "paramsgui.h"
#include "ui_paramsgui.h"

ParamsGUI::ParamsGUI(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ParamsGUI)
{
    ui->setupUi(this);

}

ParamsGUI::~ParamsGUI()
{
    delete ui;
}

void ParamsGUI::receiveParameters(QMap<QString, boost::multiprecision::mpz_int> parameters)
{
    QPalette palette;
    palette.setColor(QPalette::Base,Qt::lightGray);
    ui->lineEdit_alpha->setText(QString::fromStdString(parameters["alpha"].str()));
    ui->lineEdit_alpha->setPalette(palette);
    ui->lineEdit_beta->setText(QString::fromStdString(parameters["beta"].str()));
    ui->lineEdit_beta->setPalette(palette);
    ui->lineEdit_delta->setText(QString::fromStdString(parameters["delta"].str()));
    ui->lineEdit_delta->setPalette(palette);
    ui->lineEdit_y->setText(QString::fromStdString(parameters["y"].str()));
    ui->lineEdit_y->setPalette(palette);
    ui->lineEdit_totdelta->setText(QString::fromStdString(parameters["totient_delta"].str()));
    ui->lineEdit_totdelta->setPalette(palette);
}
