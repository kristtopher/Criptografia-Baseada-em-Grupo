#ifndef MQTTSERVER_H
#define MQTTSERVER_H

#include "iot-key-agreement-core_global.h"
#include <QtMqtt/QMqttClient>

class IOTKEYAGREEMENTCORE_EXPORT MQTTServer: public QObject
{
    Q_OBJECT
public:
    MQTTServer(const QString host, const quint16 port, const QString username = 0, const QString password = 0);

    bool subscribe(const QString topic, quint8 qos = 0);

    bool publish(const QString topic, const QString message, quint8 qos = 0, bool retain = false);

    void connectToBroker();

    void disconnectFromBroker();

    int state();

    void setHost(const QString &value);

    void setUsername(const QString &value);

    void setPassword(const QString &value);

    void setPort(const quint16 &value);

    void setIdMqtt(const QString &value);

public slots:
    void onMessageReceived(const QByteArray &message, const QMqttTopicName &topic);

    void onMessageSent(qint32 id);

    void brokerDisconnected();

    void brokerConnected();

    void updateLogStateChange();

    void onAuthenticationRequest(const QMqttAuthenticationProperties &p);

signals:
    void messageReceived(const QByteArray &message, const QMqttTopicName &topic);

    void logMessage(const QString &message);

    void connected();
private:
    QMqttClient *m_client;
    QString host, username, password, idMqtt;
    quint16 port;
};

#endif // MQTTSERVER_H
