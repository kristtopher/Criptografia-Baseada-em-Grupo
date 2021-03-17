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
#include "FFT.h"

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
        emit sessionKeyComputed("groupkey", QString::fromStdString(group_key.str()));
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
            if(users_keys.contains(message_content)){
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

        auto ecg = read_ECG(1, time_shift++);
        session_key = compute_session_key_ERIKA(ecg);
        users_keys["Trevor"] = QString::fromStdString(session_key.str());
        emit sessionKeyComputed("Trevor", users_keys["Trevor"]);

        mpz_int user_session_key = mpz_int(users_keys[message_content].toStdString());
        users_keys.remove(message_content);
        if(n_users-- >= 2)group_key ^= user_session_key;
        m_mqtt->publish(PARAM_SESSIONKEY, QString::fromStdString(group_key.str()), 2);
        emit sessionKeyComputed("groupkey", QString::fromStdString(group_key.str()));
        users.erase(std::remove_if(users.begin(), users.end(), [&message_content](const QString user){
            return (user == message_content);
        }));
        Device::n_devices--;
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

    auto ecg = read_ECG(1, time_shift++);
    session_key = compute_session_key_ERIKA(ecg);
    users_keys["Trevor"] = QString::fromStdString(session_key.str());
    emit sessionKeyComputed("Trevor", users_keys["Trevor"]);
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

void Trevor::xtea_encrypt(const void *pt, void *ct, uint32_t *skey) {
    uint8_t i;
    uint32_t v0=((uint32_t*)pt)[0], v1=((uint32_t*)pt)[1];
    uint32_t sum=0, delta=0x9E3779B9;
    for(i=0; i<32; i++) {
        v0 += ((v1 << 4 ^ v1 >> 5) + v1) ^ (sum + ((uint32_t*)skey)[sum & 3]);      //8
        sum += delta;                                                               //1
        v1 += ((v0 << 4 ^ v0 >> 5) + v0) ^ (sum + ((uint32_t*)skey)[sum>>11 & 3]);  //9
    }
    ((uint32_t*)ct)[0]=v0; ((uint32_t*)ct)[1]=v1;
}

void Trevor::xtea_decrypt(const void *ct, void *pt, uint32_t *skey) {
    uint8_t i;
    uint32_t v0=((uint32_t*)ct)[0], v1=((uint32_t*)ct)[1];
    uint32_t sum=0xC6EF3720, delta=0x9E3779B9;
    for(i=0; i<32; i++) {
        v1 -= ((v0 << 4 ^ v0 >> 5) + v0) ^ (sum + ((uint32_t*)skey)[sum>>11 & 3]);
        sum -= delta;
        v0 -= ((v1 << 4 ^ v1 >> 5) + v1) ^ (sum + ((uint32_t*)skey)[sum & 3]);
    }
    ((uint32_t*)pt)[0]=v0; ((uint32_t*)pt)[1]=v1;
}

//ERIKA

__uint32_t linearQ(__uint32_t *window){
    __uint32_t word;
    for (uint16_t k = 0; k < 64; k = k + 2) {
        if ((window[k] >> 8) & 0x1) {
            word = (word << 1);
            word = word | 0x1;
        } else word = (word << 1);
    }
    return word;
}

void byteToBin(byte n){
    unsigned i;
    for (i = 1 << 7; i > 0; i = i / 2)
        (n & i)? printf("1"): printf("0");
}

void uint32ToBin(__uint32_t n){
    unsigned i;
    for (i = 1 << 31; i > 0; i = i / 2)
        (n & i)? printf("1"): printf("0");
    printf("\n");
}

__uint32_t floatToUint32(float f){
    int length = sizeof(float);
    int lenB = length;
    byte bytes[length];
    __uint32_t word;
    for(int i = 0; i < length; i++)
        bytes[--lenB] = ((byte *) &f)[i];
    for (int j = 0; j < length; ++j)
        word = (word << 8) | bytes[j];
    return word;
}

int8_t * Trevor::read_ECG(int node, int offset){
    int8_t *myEcg = new int8_t[500];
    FILE *fpa;
    char buff[255];
    std::string filename = "p" + std::to_string(node) + "r1.txt";
    int end = 650 + offset*50;
    int id_offset = 150 + offset*50;

    fpa = fopen(filename.c_str(), "r");
    for (int m = 0; m < end; ++m) {
        fgets(buff, 255, (FILE *) fpa);
        if (m >= id_offset)
            myEcg[m-id_offset] = std::stoi(buff);
    }
    fclose(fpa);
    return myEcg;
}

boost::multiprecision::mpz_int Trevor::compute_session_key_ERIKA(int8_t *myEcg){
    mpz_int result = 0;
    float vReal[samples];
    float vImag[samples];
    uint8_t key[16];
    uint8_t pkey[16];
    //int8_t myEcg[] = {-2, -3, -2, -2, -2, 0, -1, -2, 0, 0, 1, 2, 0, 1, 2, 4, 3, 4, 5, 4, 2, 2, 1, 0, 1, 0, -1, -1, -2, -1, -2, 0, -2, -2, 0, 0, -2, -4, -2, -2, -2, -2, -1, -2, -3, -2, -2, -2, -2, 19, -8, -2, -2, -2, -2, 0, -2, -2, -2, -2, -2, -2, -3, -2, -3, -1, -3, -2, -4, -5, -4, -3, -2, -4, 0, 11, 26, 34, 23, 8, 0, 1, 0, 0, -2, -2, -3, -5, -2, -4, -5, -3, -2, -3, -2, -3, -2, -2, -1, 0, -2, -1, 0, 0, -1, 0, 0, 2, 2, 2, 3, 2, 4, 2, 3, 2, 1, 1, 0, 0, 0, -1, -2, -1, -3, -2, -2, -2, -2, -1, -2, -1, -2, -4, -2, -2, 0, -2, -2, -4, -3, -2, -3, 17, -9, -2, -1, -3, -2, -3, -1, -2, -3, -2, -2, -3, -4, -2, -4, -2, -3, -5, -4, -4, -5, -4, -4, -2, -2, 11, 24, 33, 24, 8, -2, -3, 1, -2, -1, -2, -4, -3, -5, -3, -4, -4, -2, -3, -2, -3, -2, -1, -2, -2, -2, -1, 0, 0, 1, 0, 0, 2, 1, 3, 2, 4, 4, 3, 2, 4, 1, 2, 0, -1, 0, 0, -2, -2, -1, -2, -3, -2, -2, 0, -2, -2, -2, -2, -2, -3, -2, -2, -2, -3, -2, -2, -2, 22, -11, -3, -2, -3, -2, -2, -1, -2, -3, -2, -3, -2, -3, -2, -3, -2, -2, -4, -5, -3, -4, -3, -5, -2, -3, 8, 21, 32, 33, 10, 0, -2, 1, 0, -2, -4, -2, -1, -4, -5, -2, -3, -3, -2, -2, -2, -1, -2, 0, -2, 0, -1, 0, 0, 0, 1, 1, 1, 2, 3, 4, 3, 4, 5, 4, 4, 2, 2, 0, 0, 0, 0, 0, -2, -1, -2, -2, -3, -1, -2, -1, -2, -3, 0, -1, -2, -2, -1, -2, -3, -2, -2, -2, 19, -9, -2, -2, -1, -2, -1, -2, -2, 0, -2, -1, 0, -4, -2, -1, -2, -2, -3, -2, -3, -3, -2, -4, -3, -2, 4, 17, 31, 35, 19, 6, 2, 0, 0, 0, -2, -2, -2, -3, -2, -2, -3, -2, -2, -1, -2, -2, -1, 0, -2, 0, 0, 0, 0, 0, 0, 2, 2, 2, 3, 3, 3, 4, 4, 5, 4, 2, 3, 2, 0, 0, 0, -1, 0, 0, -1, -2, 0, -1, 0, -1, -1, 0, -2, -1, 0, -1, -1, -1, -1, 0, 0, -3, -2, 13, -5, -3, -2, -2, 0, 0, -1, 0, -1, 0, -2, -1, -2, 0, -2, 0, -2, -2, -4, -4, -3, -2, -2, -1, 0, 13, 28, 37, 25, 11, 2, 0, 1, -1, -2, -2, -3, -4, -2, -2, -3, -2, -2, -3, -2, -2, -2, 0, -1, 0, -1, 0, 0, 0, 0, 1, 1, 2, 1, 3, 3, 3, 4, 4, 4, 4, 2, 1, 0, 0, 0, -1, -2, -2, -1, -2, -2, -2, -1, -2, -1, -2, -1, -1, -2, -2, -1, -2, -2, -2, -2, -2, -2, 17, -8, -2, -3, -1, -2, 0, -1, 0, -3, -2, -1, -2, -2, -1, -2, -2, -1, -4, -4, -5, -3, -4, -2, -4, -1, 9, 21, 34, 35, 10, 0, -1, 0, -1, -2, -2, -2, -4, -5, -3, -2, -2, -2, -3, -2, -2, -1, -1, 0, -2, 0, 0, 0, 0, 1, 0, 2, 3, 2, 2, 2, 4, 3, 4, 3, 4, 2, 1, 2, 0, -1, 0, -1, -2, -2, -1, 0, -2, 0, -2, -3, -2, -1, -2, -2, -2, -1, -2, -2, -2, -2, -3, -2, 17, -11, -2, -1, -2, 0, -2, -1, 0, -2, -1, -2, -1, -2, -2, -3, -2, -2, -2, -3, -3, -4, -5, -4, -5, -1, 6, 20, 32, 33, 13, 1, -2, 0, -1, -2};

    __uint32_t features[samples / 2];
    __uint32_t words = 0;
    int d = 0;
    FFT _FFT = FFT();
    for (uint16_t i = 0; i < 4; ++i) {
        d = 0;
        for (uint16_t j = 0; j < samples; j++)
            vReal[j] = vImag[j] = 0.0;//Imaginary part must be zeroed in case of looping to avoid wrong calculations and overflows
        for (int j = 0; j < 125; j++)
            vReal[j] = myEcg[j + (i * 125)];/* Build data with positive and negative values*/
        for (int j = 0; j < samples / 2; j++) features[j] = 0;

        _FFT.Windowing(vReal, samples);  /* Weigh data */
        _FFT.Compute(vReal, vImag, samples); /* Compute FFT */
        //_FFT.ComplexToMagnitude(vReal, vImag, samples); /* Compute magnitudes */

        for (int k = 0; k < 64; ++k) features[k] = floatToUint32(vReal[k]);

        words = linearQ(features);
        //words = exponentialQ(features);
        uint32ToBin(words);
        for (int l = 3; l >= 0; l--) {
            key[l + (i * 4)] = uint8_t((words >> d) & 0xff);
            d += 8;
        }
    }

    words = 0;

    for (int k = 0; k < 16; ++k) {
        if(k<4) {
            pkey[k] = key[12+k];
        } else
            pkey[k] = key[k-4];
    }

    for (int g = 0; g < 16; g++) {
        key[g] = key[g] ^ pkey[g];
        result = result << 8;
        result = result | key[g];
    }

    return result;
}
