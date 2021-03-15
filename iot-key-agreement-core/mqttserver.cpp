#include "mqttserver.h"
#include <QDateTime>

MQTTServer::MQTTServer(const QString _host, const quint16 _port, const QString _username, const QString _password):
    host(_host),
    username(_username),
    password(_password),
    port(_port)
{
    m_client = new QMqttClient();
    m_client->setHostname(host);
    m_client->setPort(port);

    if(!username.isEmpty()){
        m_client->setUsername(username);
        m_client->setPassword(password);
    }

    QObject::connect(m_client, &QMqttClient::authenticationRequested, this, &MQTTServer::onAuthenticationRequest);
    QObject::connect(m_client, &QMqttClient::messageReceived, this, &MQTTServer::onMessageReceived);
    QObject::connect(m_client, &QMqttClient::stateChanged, this, &MQTTServer::updateLogStateChange);
    QObject::connect(m_client, &QMqttClient::disconnected, this, &MQTTServer::brokerDisconnected);
    QObject::connect(m_client, &QMqttClient::connected, this, &MQTTServer::brokerConnected);
}

bool MQTTServer::subscribe(const QString topic, quint8 qos)
{
    auto subscription = m_client->subscribe(topic, qos);

    if(!subscription){
        qWarning() << "Warning: could not subscribe to the topic \'" << topic << "\'.\n";
        return false;
    }
    return true;
}

bool MQTTServer::publish(const QString topic, const QString message, quint8 qos, bool retain)
{
    if(m_client->publish(topic, message.toUtf8(), qos, retain) == -1){
        qWarning() << "Warning: a message could not be published to the topic \'" << topic << "\'.\n";
        return false;
    }
    return true;
}

void MQTTServer::connectToBroker()
{
    if(m_client->state() == QMqttClient::Disconnected){
        m_client->setHostname(host);
        m_client->setPort(port);
        m_client->setUsername(username);
        m_client->setPassword(password);
        m_client->connectToHost();
    }
}

void MQTTServer::disconnectFromBroker()
{
    if(m_client->state() == QMqttClient::Connected){
        m_client->disconnectFromHost();
    }
}

int MQTTServer::state()
{
    return m_client->state();
}

void MQTTServer::onMessageReceived(const QByteArray &message, const QMqttTopicName &topic)
{
    if(idMqtt.isEmpty()){
        idMqtt = "Server";
    }
    const QString content = QString("[") + idMqtt + QString("]: ") +  QDateTime::currentDateTime().toString()
            + QLatin1String(" Received Topic: ")
            + topic.name()
            + QLatin1String(" Message: ")
            + message
            + QLatin1Char('\n');
    //qDebug() << content;
    emit logMessage(content);
    emit messageReceived(message, topic);
}

void MQTTServer::onMessageSent(qint32 id)
{
    QString content;
    content = QString("Message sent with id ") + QString::number(id) + QString(".");
    emit logMessage(content);
}

void MQTTServer::brokerDisconnected()
{
    qDebug() << "Log: broker disconnected.\n";
    emit logMessage("Log: broker disconnected.");
}

void MQTTServer::brokerConnected()
{
    emit connected();
    qDebug() << "Log: broker connected.\n";
    emit logMessage("Log: broker connected.");
}

void MQTTServer::updateLogStateChange()
{
    const QString content = QDateTime::currentDateTime().toString()
                        + QLatin1String(": State Change ")
                        + QString::number(m_client->state());
    qDebug() << content;
    emit logMessage(content);
}

void MQTTServer::onAuthenticationRequest(const QMqttAuthenticationProperties &p)
{
    qDebug() << "[MQTTServer]: Authentication requested.\n";
}

void MQTTServer::setIdMqtt(const QString &value)
{
    m_client->setClientId(value);
    idMqtt = value;
}

void MQTTServer::setPort(const quint16 &value)
{
    port = value;
}

void MQTTServer::setPassword(const QString &value)
{
    password = value;
}

void MQTTServer::setUsername(const QString &value)
{
    username = value;
}

void MQTTServer::setHost(const QString &value)
{
    host = value;
}
