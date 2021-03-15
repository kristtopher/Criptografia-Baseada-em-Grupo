#ifndef TREVOR_H
#define TREVOR_H

#include "iot-key-agreement-core_global.h"
#include "mqttserver.h"

#include <QObject>
#include <QMap>
#include <QQueue>
#include <QVector>
#include <QElapsedTimer>
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

    void generateSessionParameters();

    void setNewSession();

    std::vector<int64_t> segmented_sieve(int64_t limit);

    void setHost(const QString &value);

    void setUsername(const QString &value);

    void setPassword(const QString &value);

    void setPort(const quint16 &value);

    void setM(const size_t &value);

    void setN(const size_t &value);

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

private:
    QString host, username, password;
    QString time_type = "Individual";
    quint16 port;
    size_t n_users = 0, m = 0, n = 0;
    bool sess_params_computed = false;
    MQTTServer *m_mqtt;
    QVector<QString> users;
    QVector<QElapsedTimer> users_timer;
    double time_counter = 0, max_time = 0;
    QVector<bool> user_sess_computed;
    QMap<QString, boost::multiprecision::mpz_int> params;
    QQueue<QString> users_queue;

    void pattern_one(std::vector<int64_t> &pm, std::vector<int64_t> &pn, const std::vector<int64_t> &primes);
    void pattern_two(std::vector<int64_t> &pm, std::vector<int64_t> &pn, const std::vector<int64_t> &primes);
    void connect_user(const QString& user);
};

#endif // TREVOR_H
