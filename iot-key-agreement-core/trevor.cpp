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
const QString PARAM_ALPHA = "dcc075/params/alpha";
const QString PARAM_BETA = "dcc075/params/beta";
const QString PARAM_Y = "dcc075/params/y_param";
const QString PARAM_DELTA = "dcc075/params/delta";
const QString PARAM_TOTIENTDELTA = "dcc075/params/totient_delta";
const QString SESSION_KEY = "dcc075/sessionkey";

const int64_t L1D_CACHE_SIZE = 32768;

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
        generateSessionParameters();
        emit sessionParamsComputed(params);
    }else if(n_users > 2){
        for(int i = 0; i < users_timer.size(); i++){
            users_timer[i].restart();
        }
    }

    if(sess_params_computed){
       qDebug() << "\n Parameters: \n";
       qDebug() << "Y: " << QString::fromStdString(params["y"].str());
       qDebug() << "Alpha: " << QString::fromStdString(params["alpha"].str());
       qDebug() << "Beta: " << QString::fromStdString(params["beta"].str());
       qDebug() << "Delta: " << QString::fromStdString(params["delta"].str());
       qDebug() << "Totient delta: " << QString::fromStdString(params["totient_delta"].str());

       qDebug() << "\nSending session parameters.\n";

       QString param_y = PARAM_Y + QString("_") + user, param_alpha = PARAM_ALPHA + QString("_") + user;
       QString param_beta = PARAM_BETA + QString("_") + user, param_delta = PARAM_DELTA + QString("_") + user;
       QString param_totient = PARAM_TOTIENTDELTA + QString("_") + user;

       m_mqtt->publish(param_y,QString::fromStdString(params["y"].str()), 2);
       m_mqtt->publish(param_alpha,QString::fromStdString(params["alpha"].str()), 2);
       m_mqtt->publish(param_beta,QString::fromStdString(params["beta"].str()), 2);
       m_mqtt->publish(param_delta,QString::fromStdString(params["delta"].str()), 2);
       m_mqtt->publish(param_totient, QString::fromStdString(params["totient_delta"].str()), 2);
    }
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
            if(n_users == 1 || !not_enter){
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

        if(!users_queue.isEmpty() && i == user_sess_computed.size()){
            QString next_user = users_queue.dequeue();
            connect_user(next_user);
            for(auto _user: users){
                if(user == _user) continue;
                m_mqtt->publish(COMMAND_USER, _user + QString("_sendgamma"), 2);
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

mpz_int phi(mpz_int n)
{
    mpz_int ret = 1,i,pow;
    for (i = 2; n != 1; i++)
    {
        pow = 1;
        if(i>boost::multiprecision::sqrt(n))break;
        while (!(n%i))
        {
            n /= i;
            pow *= i;
        }
        ret *= (pow - (pow/i));
    }
    if(n!=1)ret*=(n-1);
    return ret;
}

void Trevor::pattern_one(std::vector<int64_t> &pm, std::vector<int64_t> &pn, const std::vector<int64_t> &primes){
    std::vector<size_t> primes_ids(primes.size());
    size_t i;
    mpz_int temp_q = 1;
    std::random_device rd;
    mt19937 mt(rd());
    uniform_int_distribution<size_t> uid(0, primes.size());

    std::generate(primes_ids.begin(), primes_ids.end(), [&mt, &uid](){
        return uid(mt);
    });

    for(i = 0; i < m; i++){
        int64_t p = primes[primes_ids[i]];
        pm.push_back(p);
        temp_q *= p;
    }

    for(; i < (m + n); i++){
        int64_t p = primes[primes_ids[i]];
        pn.push_back(p);
        mpz_int p_power = 1;
        for(size_t j = 0; j < 3; j++){
            p_power *= p;
        }
        params["delta"] *= p_power;
        params["alpha"] *= (p_power / p) * (p-1);
        params["beta"] *= p*(p-1)*temp_q;
    }
}

void Trevor::pattern_two(std::vector<int64_t> &pm, std::vector<int64_t> &pn, const std::vector<int64_t> &primes){
    std::vector<size_t> primes_ids(primes.size());
    size_t i;
    mpz_int temp_q = 1;

    std::iota(primes_ids.begin(), primes_ids.end(), 0);
    std::random_shuffle(primes_ids.begin(), primes_ids.end());

    for(i = 0; i < m; i++){
        int64_t p = primes[primes_ids[i]];
        pm.push_back(p);
        params["beta"] *= p;
    }

    for(; i < (m + n); i++){
        int64_t p = primes[primes_ids[i]];
        pn.push_back(p);
        params["delta"] *= p;
        params["alpha"] *= (p-1);
    }

}

void Trevor::generateSessionParameters()
{
    params["delta"] = 1;
    params["alpha"] = 1;
    params["beta"] = 1;
    params["totient_delta"] = 0;
    params["y"] = 0;

    std::vector<int64_t> pm, pn;

    int64_t limit = 10000;
    auto primes = segmented_sieve(limit);

    while(primes.size() < (m+n)) primes = segmented_sieve(limit);

    pattern_two(pm, pn, primes);
    mt19937 mt;
    uniform_int_distribution<mpz_int> uid(0, params["delta"]);

    params["y"] = uid(mt);
    mpz_int g;
    g = boost::multiprecision::gcd(params["y"], params["delta"]);
    while(g != 1){
        g = boost::multiprecision::gcd(params["y"], params["delta"]);
        params["y"] = uid(mt);
    }
    params["totient_delta"] = phi(params["delta"]);
    sess_params_computed = true;
}

void Trevor::setNewSession()
{
    sess_params_computed = false;
}

std::vector<int64_t> Trevor::segmented_sieve(int64_t limit)
{
    int64_t sqrt = (int64_t) std::sqrt(limit);
    int64_t segment_size = std::max(sqrt, L1D_CACHE_SIZE);
    int64_t count = (limit < 2) ? 0 : 1;

    // we sieve primes >= 3
    int64_t i = 3;
    int64_t n = 3;
    int64_t s = 3;

    std::vector<char> sieve(segment_size);
    std::vector<char> is_prime(sqrt + 1, true);
    std::vector<int64_t> primes;
    std::vector<int64_t> multiples;

    for (int64_t low = 0; low <= limit; low += segment_size)
    {
    std::fill(sieve.begin(), sieve.end(), true);

    // current segment = [low, high]
    int64_t high = low + segment_size - 1;
    high = std::min(high, limit);

    // generate sieving primes using simple sieve of Eratosthenes
    for (; i * i <= high; i += 2)
      if (is_prime[i])
        for (int64_t j = i * i; j <= sqrt; j += i)
          is_prime[j] = false;

    // initialize sieving primes for segmented sieve
    for (; s * s <= high; s += 2)
    {
      if (is_prime[s])
      {
           primes.push_back(s);
        multiples.push_back(s * s - low);
      }
    }

    // sieve the current segment
    for (std::size_t i = 0; i < primes.size(); i++)
    {
      int64_t j = multiples[i];
      for (int64_t k = primes[i] * 2; j < segment_size; j += k)
        sieve[j] = false;
      multiples[i] = j - segment_size;
    }
    primes.clear();
    for (; n <= high; n += 2)
      if (sieve[n - low]) // n is a prime
        primes.push_back(n);
    }

    return primes;
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
