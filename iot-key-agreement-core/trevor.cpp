#include <iostream>
#include <algorithm>
#include <cmath>
#include <random>
#include <vector>
#include <cstdlib>
#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include<boost/multiprecision/number.hpp>
#include <boost/random.hpp>

#include "trevor.h"

using namespace boost::multiprecision;
using namespace boost::random;


const QString CONNECT_USER = "dcc075/users/connect";
const QString DISCONNECT_USER = "dcc075/users/disconnect";
const QString NUMBER_USERS = "dcc075/users/number_users";
const QString COMMAND_USER = "dcc075/users/command";
const QString PARAM_SESSIONKEY = "ERIKA/params/sessionkey";
const QString SESSION_KEY = "ERIKA/sessionkey";

Trevor::Trevor(const QString host, const quint16 port, const QString username, const QString password)
{
    this->init(host, port, username, password);
}

void Trevor::init(const QString host, const quint16 port, const QString username, const QString password)
{
    if(!username.isEmpty()){
        m_mqtt = new MQTTServer(host, port, username, password);
    }else {
        m_mqtt = new MQTTServer(host, port);
    }

    QObject::connect(m_mqtt, &MQTTServer::messageReceived, this, &Trevor::onMessageReceived);
    QObject::connect(m_mqtt, &MQTTServer::connected, this, &Trevor::subscribeToTopics);
    QObject::connect(m_mqtt, &MQTTServer::logMessage, this, &Trevor::sendLogToGUI);
}

void Trevor::dropUsers()
{
    for(auto user: users){
        emit userDisconnected(user);
        qDebug() << user << " disconnected.\n";
    }
    users.erase(std::remove_if(this->users.begin(), this->users.end(), [](QString _user){
        return true;
    }), users.end());
    n_users = 0;
}

void Trevor::connectToHost()
{
    if(m_mqtt->state() == QMqttClient::Connected){
        m_mqtt->disconnectFromBroker();
    }
    m_mqtt->setHost(host);
    m_mqtt->setPort(port);
    m_mqtt->setUsername(username);
    m_mqtt->setPassword(password);
    m_mqtt->connectToBroker();
}

void Trevor::disconnectFromHost()
{
    if(m_mqtt->state() == QMqttClient::Connected)
        m_mqtt->disconnectFromBroker();
}

void Trevor::connect_user(const QString& user)
{
    m_mqtt->publish(COMMAND_USER, user + QString("_accepted"), 2);
    users.push_back(user);
    users_timer.resize(users.size());
    users_timer[users_timer.size()-1].start();
    user_sess_computed.push_back(false);
    user_sess_computed.fill(false);
    n_users++;
    m_mqtt->publish(NUMBER_USERS, QString::number(n_users), 2);
    qDebug() << user << " connected.\n";
    emit userConnected(user);

    if(n_users == 1){
        qDebug() << "\nGenerating session parameters...\n";

        emit sessionParamsComputed(params);
    }else if(n_users >= 2){
        // misturar chaves e enviar para dispositivos (broadcast)
        mpz_int mixed_session_key = mpz_int(users_keys[*(users_keys.keyBegin())].toStdString());
        int i = 0;
        for(auto key_it = users_keys.keyBegin(); key_it != users_keys.keyEnd(); key_it++){
            if(i == 0){
                i++;
                continue;
            }
            QString str_session_key = users_keys[*key_it];
            auto session_key = mpz_int(str_session_key.toStdString());

            mixed_session_key ^= session_key;
        }
        group_key = mixed_session_key;
        m_mqtt->publish(PARAM_SESSIONKEY, QString::fromStdString(mixed_session_key.str()), 2);
        for(int i = 0; i < users_timer.size(); i++){
            users_timer[i].restart();
        }
    }

//    if(sess_params_computed){
//      // ver se tem algo para fazer aqui
//    }
}

void Trevor::onMessageReceived(const QByteArray &message, const QMqttTopicName &topic)
{
    QString topic_name = topic.name();
    QString message_content = QString::fromUtf8(message.data());

    if(message_content.isEmpty()) return;

    if(topic_name == CONNECT_USER){
        bool already_in = false;
        for(auto user: users){
            if(user == message_content){
                already_in = true;
                break;
            }
        }
        if(!already_in){
            int i;
            bool not_enter = false;
            for(i = 0; i < user_sess_computed.size(); i++){
                if(!user_sess_computed[i]){
                    not_enter = true;
                    break;
                }
            }
            if(users_keys.contains(message_content) && (n_users == 1 || !not_enter)){
                connect_user(message_content);
            }else users_queue.enqueue(message_content);
        }else {
            qDebug() << message_content << " is already connected.\n";
        }
    }
    if(topic_name == SESSION_KEY){
        QStringList pieces = message_content.split("_");
        QString user = pieces[0], session_key = pieces[1];
        int i;

        for(i = 0; i < users.size(); i++){
            if(users[i] == user){
                break;
            }
        }
        if(i < users.size()) user_sess_computed[i] = true;

        for(i = 0; i < user_sess_computed.size(); i++){
            if(!user_sess_computed[i]){
                break;
            }else{
                double time_elapsed = double(users_timer[i].elapsed());
                if(time_elapsed > max_time){
                    max_time = time_elapsed/1000;
                }
            }
        }
        qDebug() << "i: " << i << "users: " << user_sess_computed.size();
        if(i == user_sess_computed.size()){
            qDebug() << time_counter;
            if(time_type == "Individual"){
                time_counter = max_time;
            }else if(time_type == "Cummulative"){
                time_counter += max_time;
            }
            emit sessionTime(time_counter, users.size());
            if(time_type == "Individual"){
                time_counter = 0.0;
            }
        }

        users_keys[user] = session_key;

        if(!users_queue.isEmpty() && i == user_sess_computed.size()){
            while(!users_queue.empty()){
                QString next_user = users_queue.dequeue();
                if(users_keys.contains(next_user)){
                    connect_user(next_user);
                }else{
                    users_queue.enqueue(next_user);
                }
            }
            for(auto _user: users){
                if(user == _user) continue;
               // m_mqtt->publish(COMMAND_USER, _user + QString("_sendgamma"), 2);
            }
        }

        emit sessionKeyComputed(user, session_key);
        qDebug() << "\n" << user + " session key: "  << session_key << "\n";
    }
    if(topic_name == DISCONNECT_USER){
        int i;
        for(i = 0; i < users.size(); i++){
            if(users[i] == message_content) break;
        }
        if(i == users.size()){
            qWarning() << "\n" << message_content << "is not an user.\n";
            return;
        }

        mpz_int user_session_key = mpz_int(users_keys[message_content].toStdString());
        users_keys.remove(message_content);
        group_key ^= user_session_key;
        m_mqtt->publish(PARAM_SESSIONKEY, QString::fromStdString(group_key.str()), 2);

        std::remove_if(users.begin(), users.end(), [&message_content](const QString user){
            return (user == message_content);
        });
        n_users--;
        emit userDisconnected(message_content);
        qDebug() << message_content << " disconnected.\n";
    }
}

void Trevor::subscribeToTopics()
{
    m_mqtt->publish(CONNECT_USER, "", 2, true);
    m_mqtt->subscribe(CONNECT_USER, 2);
    m_mqtt->publish(DISCONNECT_USER, "", 2, true);
    m_mqtt->subscribe(DISCONNECT_USER, 2);
    m_mqtt->publish(SESSION_KEY, "", 2, true);
    m_mqtt->subscribe(SESSION_KEY, 2);

    emit serverConnected();
}

void Trevor::sendLogToGUI(const QString &msg)
{
    emit emitLogMessage(msg);
}

void Trevor::changeMeasurementType(const QString &measurement_type)
{
    time_type = measurement_type;
}

MQTTServer *Trevor::getMqtt() const
{
    return m_mqtt;
}

QMap<QString, boost::multiprecision::mpz_int> Trevor::getParams() const
{
    return params;
}


void Trevor::setNewSession()
{
    sess_params_computed = false;
}


void Trevor::setHost(const QString &value)
{
    host = value;
}

void Trevor::setUsername(const QString &value)
{
    username = value;
}

void Trevor::setPassword(const QString &value)
{
    password = value;
}

void Trevor::setPort(const quint16 &value)
{
    port = value;
}


void Trevor::setN(const size_t &value)
{
    n = value;
}

void Trevor::setM(const size_t &value)
{
    m = value;
}
