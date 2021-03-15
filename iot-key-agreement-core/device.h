#ifndef DEVICE_H
#define DEVICE_H

#include "iot-key-agreement-core_global.h"
#include "mqttserver.h"
#include <QObject>
#include <QElapsedTimer>
#include <boost/multiprecision/gmp.hpp>

class IOTKEYAGREEMENTCORE_EXPORT Device: public QObject
{
    Q_OBJECT
public:
    Device(const QString &host, const int port, const QString &user, const QString &password);
    ~Device();
    QString getId_mqtt() const;

    void setN_cobaias(const size_t &value);

private:
    int8_t * read_ECG(int node);
    boost::multiprecision::mpz_int compute_session_key_ERIKA(int8_t *myEcg);

    std::string generate_random_id(std::string &str, size_t len);
    void xtea_encrypt(const void *pt, void *ct, uint32_t *skey);
    void xtea_decrypt(const void *ct, void *pt, uint32_t *skey);

signals:
    void emitTotalTime(int totalTime);

private slots:
    void onMessageReceived(const QByteArray &message, const QMqttTopicName &topic);
    void subscribeToTopics();

private:
    const QString CONNECT_USER = "dcc075/users/connect";
    const QString DISCONNECT_USER = "dcc075/users/disconnect";
    const QString NUMBER_USERS = "dcc075/users/number_users";
    const QString COMMAND_USER = "dcc075/users/command";
    const QString SESSION_KEY = "dcc075/sessionkey";
    QString PARAM_SESSIONKEY = "ERIKA/params/sessionkey";


private:
    MQTTServer *m_mqtt;
    QString id_mqtt;
    QVector<QString> users;
    boost::multiprecision::mpz_int session_key, group_key;
    size_t n_users = 0, server_id = 0, n_cobaias = 0;
    bool session_key_computed = false, on_experimentation = false;
    bool accepted = false;
    QElapsedTimer timer;
    int total_time = 0;

public:
    static size_t n_devices;

};

#endif // DEVICE_H
