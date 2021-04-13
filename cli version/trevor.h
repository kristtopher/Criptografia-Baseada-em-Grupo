#ifndef TREVOR_H
#define TREVOR_H

#include "iot-key-agreement-core_global.h"
#include "mqttserver.h"
#include "device.h"

#include <QObject>
#include <QMap>
#include <QQueue>
#include <QVector>
#include <QElapsedTimer>
#include<boost/multiprecision/number.hpp>
#include <boost/multiprecision/gmp.hpp>

class IOTKEYAGREEMENTCORE_EXPORT Trevor: public QObject
{
    Q_OBJECT
public:
    Trevor(){};

    Trevor(const QString host, const quint16 port, const QString username = 0, const QString password = 0);

    void init(const QString host, const quint16 port, const QString username = 0, const QString password = 0);

    void dropUsers();

    void connectToHost();

    void disconnectFromHost();

    void setNewSession();

    void setHost(const QString &value);

    void setUsername(const QString &value);

    void setPassword(const QString &value);

    void setPort(const quint16 &value);

    virtual ~Trevor(){ };

    QMap<QString, boost::multiprecision::mpz_int> getParams() const;

    MQTTServer *getMqtt() const;

public slots:
    void onMessageReceived(const QByteArray &message, const QMqttTopicName &topic);

    void subscribeToTopics();

    void sendLogToGUI(const QString &msg);

    void changeMeasurementType(const QString &measurement_type);

signals:
    void userConnected(const QString user);

    void userDisconnected(const QString user);

    void sessionKeyComputed(const QString user, const QString session_key);

    void sessionTime(double time, int n_users);

    void emitLogMessage(const QString &msg);

    void sessionParamsComputed(QMap<QString, boost::multiprecision::mpz_int>);

    void serverConnected();

    void empty();

    void energyConsumption(size_t device_id, double time, double consumption);

private:
    QString host, username, password;
    QString time_type = "Individual";
    quint16 port;
    size_t n_users = 0, n_key = 0;
    bool sess_params_computed = false;
    MQTTServer *m_mqtt;
    QVector<QString> users;
    QElapsedTimer time;
    double time_counter = 0, max_time = 0;
    double energy_used = 0.1;
    double V = 3.7, I = 0.0174; // Tensão (V) e corrente (I)
    double Ti = 0; // Last time
    double time_server_connect = 0.0;
    QVector<bool> user_sess_computed;
    QMap<QString, boost::multiprecision::mpz_int> params;
    QMap<QString, QString> users_keys;
    QQueue<QString> users_queue;
    boost::multiprecision::mpz_int group_key, trevor_session_key;
    int key_shift = 0;


    void connect_user(const QString& user);
    boost::multiprecision::mpz_int compute_session_key_ERIKA(int8_t *myEcg);
    int8_t * read_ECG(int node, int offset);
    void xtea_encrypt(const void *pt, void *ct, uint32_t *skey);
    void xtea_decrypt(const void *ct, void *pt, uint32_t *skey);
    void update_key();
    double compute_energy_consumption();
};

#endif // TREVOR_H
