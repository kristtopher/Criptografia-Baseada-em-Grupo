#include "device.h"
#include <boost/random.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <random>
#include <string>
#include <gmpxx.h>
#include "FFT.cpp"


using namespace boost::multiprecision;
using namespace boost::random;

size_t Device::n_devices = 0;

Device::Device(const QString &host, const int port, const QString &user, const QString &password)
{

    if(!user.isEmpty()){
        m_mqtt = new MQTTServer(host, port, user, password);
    }else {
        m_mqtt = new MQTTServer(host, port);
    }

    QObject::connect(m_mqtt, &MQTTServer::messageReceived, this, &Device::onMessageReceived);
    QObject::connect(m_mqtt, &MQTTServer::connected, this, &Device::subscribeToTopics);

    n_devices++;
    server_id = n_devices+1;

    std::string id(10, 1);
    generate_random_id(id, 10);
    id_mqtt = QString::fromStdString(id);
    qDebug() << "MQTT id: " << id_mqtt << "\n";
    m_mqtt->setIdMqtt(id_mqtt);
    m_mqtt->connectToBroker();
}

Device::~Device()
{
    m_mqtt->publish(DISCONNECT_USER, id_mqtt, 2);
}

void Device::xtea_encrypt(const void *pt, void *ct, uint32_t *skey) {
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

void Device::xtea_decrypt(const void *ct, void *pt, uint32_t *skey) {
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

__uint32_t Device::linearQ(__uint32_t *window){
    __uint32_t word;
    for (uint16_t k = 0; k < 64; k = k + 2) {
        if ((window[k] >> 8) & 0x1) {
            word = (word << 1);
            word = word | 0x1;
        } else word = (word << 1);
    }
    return word;
}

void Device::byteToBin(byte n){
    unsigned i;
    for (i = 1 << 7; i > 0; i = i / 2)
        (n & i)? printf("1"): printf("0");
}

void Device::uint32ToBin(__uint32_t n){
    unsigned i;
    for (i = 1 << 31; i > 0; i = i / 2)
        (n & i)? printf("1"): printf("0");
    printf("\n");
}

__uint32_t Device::floatToUint32(float f){
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

int8_t * Device::read_ECG(int node){
    int8_t *myEcg = new int8_t[500];
    FILE *fpa;
    char buff[255];
    std::string filename = "p" + std::to_string(node) + "r1.txt";
    //        string outfilename = "../key_p" + to_string(n) + "r1.txt";
    //        string filename = "../p1r" + to_string(n) + ".txt";
    //        string outfilename = "../key_p1r" + to_string(n) + ".txt";
    //        string filename = "../p9r1.txt";
    //        string outfilename = "../N_key_p9r1.txt";

    //fpa = fopen(filename, "r");
    fpa = fopen(filename.c_str(), "r");
    for (int m = 0; m < 650; ++m) {
        fgets(buff, 255, (FILE *) fpa);
        if (m >= 150)
            myEcg[m-150] = std::stoi(buff);
    }
    fclose(fpa);
    return myEcg;
}

boost::multiprecision::mpz_int Device::compute_session_key_ERIKA(int8_t *myEcg){
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

    m_mqtt->publish(SESSION_KEY, this->id_mqtt + QString("_") + QString::fromStdString(result.str()), 2);
    session_key_computed = true;
    return result;
}

//ERIKA

std::string Device::generate_random_id(std::string &str, size_t length)
{
    auto randchar = []() -> char{
        const char charset[] =
                "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[ rand() % max_index ];
    };
    std::generate_n(str.begin(), length, randchar);
    return str;
}

void Device::onMessageReceived(const QByteArray &message, const QMqttTopicName &topic)
{
    QString topic_name = topic.name();
    timer.start();
    QString message_content = QString::fromUtf8(message.data());
    total_time += timer.elapsed();

    if(topic_name == COMMAND_USER){
        QStringList pieces = message_content.split('_');
        if(pieces[0] != id_mqtt) return;
        if(pieces[1] == QString("accepted")){
            accepted = true;
        }

    }

    if(accepted) qDebug() << id_mqtt << " " << message << " " << topic;

    if(topic_name == NUMBER_USERS){
        n_users = message_content.toInt();
        timer.start();
        total_time += timer.elapsed();
       // if(server_id == 0) server_id = n_users;
    }
    if(topic_name == DISCONNECT_USER){
//        auto it = users.begin();
//        size_t i;
//        for(i = 0; it != users.end(); it++, i++){
//            if((*it) == message_content) break;
//        }
//        users.erase(it);
//        n_users--;
//        //if(n_users > 1 && n_users == gammas.size()) compute_session_key();
        qDebug() << QString("[") + id_mqtt + QString("]: ") + message_content << " disconnected.\n";
    }

    if(topic_name == PARAM_SESSIONKEY){
        group_key = mpz_int(message_content.toStdString());
    }

}

void Device::subscribeToTopics()
{
    m_mqtt->subscribe(NUMBER_USERS, 2);
    m_mqtt->subscribe(COMMAND_USER, 2);
    m_mqtt->subscribe(PARAM_SESSIONKEY, 2);
    m_mqtt->subscribe(DISCONNECT_USER, 2);

    auto ecg = read_ECG(server_id);
    session_key = compute_session_key_ERIKA(ecg);

    qDebug() << QString::fromStdString(session_key.str());
    m_mqtt->publish(CONNECT_USER, this->id_mqtt, 2);
}

void Device::setN_cobaias(const size_t &value)
{
    on_experimentation = true;
    n_cobaias = value;
}

QString Device::getId_mqtt() const
{
    return id_mqtt;
}
