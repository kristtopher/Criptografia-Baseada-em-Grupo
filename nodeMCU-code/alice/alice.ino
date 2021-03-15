#include <ESP8266WiFi.h> // Importa a Biblioteca ESP8266WiFi
#include <PubSubClient.h> // Importa a Biblioteca PubSubClient
#include <BigNumber.h>

//defines:
//defines de id mqtt e tópicos para publicação e subscribe
#define TOPICO_DISCONNECT   "dcc075/users/disconnect"    //tópico MQTT de envio de informações para Broker
#define TOPICO_CONNECT      "dcc075/users/connect"
#define TOPICO_NUSERS       "dcc075/users/number_users"
#define TOPICO_ALPHA        "dcc075/params/alpha"
#define TOPICO_BETA         "dcc075/params/beta"
#define TOPICO_DELTA        "dcc075/params/delta"
#define TOPICO_TOTIENT      "dcc075/params/totient_delta"
#define TOPICO_GAMMA        "dcc075/params/gamma"
#define TOPICO_GAMMA1       "dcc075/params/gamma1"
#define TOPICO_Y            "dcc075/params/y_param"
#define TOPICO_COMMAND      "dcc075/users/command"
#define SESSION_KEY_ALICE   "dcc075/sessionkey/alice"
#define SESSION_KEY_BOB     "dcc075/sessionkey/bob"

// ATENCAO: coloque um id unico.
#define ID_MQTT  "Alice"     //id mqtt (para identificação de sessão)
#define pubQoS_MQTT 2
#define subQoS_MQTT 2

//defines - mapeamento de pinos do NodeMCU
#define D0    16
#define D1    5
#define D2    4
#define D3    0
#define D4    2
#define D5    14
#define D6    12
#define D7    13
#define D8    15
#define D9    3
#define D10   1


// WIFI
const char* SSID = "TP-LINK_3319A2"; // SSID / nome da rede WI-FI que deseja se conectar
const char* PASSWORD = "3&SnaW@E$NEK"; // Senha da rede WI-FI que deseja se conectar

// MQTT
const char* BROKER_MQTT = "broker.hivemq.com";
int BROKER_PORT = 1883; // Porta do Broker MQTT


//Variáveis e objetos globais
WiFiClient espClient; // Cria o objeto espClient
PubSubClient MQTT(espClient); // Instancia o Cliente MQTT passando o objeto espClient
bool on_net = true;
bool params_received = false;
bool gamma_computed = false;
bool session_key_computed = false;
String alpha, beta, delta, totient_delta, gamma1, gamma2, y;
String xb1;
size_t n_users = 0, server_id = 0;

//Prototypes
void initSerial();
void initWiFi();
void initMQTT();
void reconectWiFi();
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void VerificaConexoesWiFIEMQTT(void);

/*
 *  Implementações das funções
 */
void setup()
{
    //inicializações:
    BigNumber::begin();
    randomSeed(analogRead(A0));
    initSerial();
    initWiFi();
    initMQTT();
}

//Função: inicializa comunicação serial com baudrate 115200 (para fins de monitorar no terminal serial
//        o que está acontecendo.
//Parâmetros: nenhum
//Retorno: nenhum
void initSerial(){
    Serial.begin(115200);
}

//Função: inicializa e conecta-se na rede WI-FI desejada
//Parâmetros: nenhum
//Retorno: nenhum
void initWiFi()
{
    delay(10);
    Serial.println("------Conexao WI-FI------");
    Serial.print("Conectando-se na rede: ");
    Serial.println(SSID);
    Serial.println("Aguarde");

    reconectWiFi();
}

//Função: inicializa parâmetros de conexão MQTT(endereço do
//        broker, porta e seta função de callback)
//Parâmetros: nenhum
//Retorno: nenhum
void initMQTT()
{
    MQTT.setServer(BROKER_MQTT, BROKER_PORT);   //informa qual broker e porta deve ser conectado
    MQTT.setCallback(mqtt_callback);            //atribui função de callback (função chamada quando qualquer informação de um dos tópicos subescritos chega)
}

int randomCharArray(char *s, char *r)
{
  bool critical = true;
  byte i = 0;
  byte len = strlen(s);
  while (critical && i < len)
  {
    byte lm = s[i] - '0';
    r[i] = random(lm + 1) + '0';
    critical = (r[i] == s[i]);
    i++;
  }

  while (i < len)
  {
    uint32_t rv = rand();       // 4 bytes = 8 nybbles 0..15
    byte sum = 0;
    for (byte b = 0; b < 8; b++)
    {
      // get nybble
      byte v = rv & 0x0F;
      // prepare for next nybble
      rv >>= 4;
      // not in 0..9 ==> skip nybble but use its value.
      if (v > 9)
      {
        sum += v;
        continue;
      }
      r[i++] = v + '0';
      if (i == len) break;
    }
    // if all nybbles are skipped, sum is at least 80
    // so we can use sum to generate at least one digit per iteration.
    if (i < len && sum >= 80)
    {
      r[i++] = sum % 10 + '0';
    }
  }
  r[len] = '\0';
}

char StrContains(const char *str, const char *sfind)
{
    char found = 0;
    char index = 0;
    char len;

    len = strlen(str);

    if (strlen(sfind) > len) {
        return 0;
    }
    while (index < len) {
        if (str[index] == sfind[found]) {
            found++;
            if (strlen(sfind) == found) {
                return 1;
            }
        }
        else {
            found = 0;
        }
        index++;
    }
    return 0;
}

BigNumber compute_session_key(BigNumber &_gamma, BigNumber &xb){
  BigNumber _delta      = (char*)delta.c_str();
  BigNumber _y          = (char*)y.c_str();
  BigNumber mult        = _gamma * xb;
  BigNumber session_key = _y.powMod(mult, _delta);
  session_key_computed  = true;
  Serial.println(String(String("\n- Session key generated by ") + ID_MQTT + String(".")));
  Serial.print("Session key: ");
  Serial.println(session_key);
  return session_key;
}

void compute_gamma1(){
  BigNumber _alpha         = (char*)alpha.c_str();
  BigNumber _beta          = (char*)beta.c_str();
  BigNumber _delta         = (char*)delta.c_str();
  BigNumber _gamma         = (char*)gamma1.c_str();
  BigNumber _totient_delta = (char*)totient_delta.c_str();

  Serial.print("Y: ");
  Serial.println(y);
  Serial.print("Alpha: ");
  Serial.println(_alpha);
  Serial.print("Beta: ");
  Serial.println(_beta);
  Serial.print("Delta: ");
  Serial.println(_delta);
  Serial.print("Totient delta: ");
  Serial.println(_totient_delta);
  char r[100] = "";
  randomCharArray((char*)delta.c_str(), r);
  BigNumber xa = r;
  char s[100] = "";
  randomCharArray((char*)totient_delta.c_str(), s);
  BigNumber xb = s;

  while(xb*_gamma % _totient_delta == 0){
    char t[100] = "";
    randomCharArray((char*)totient_delta.c_str(), t);
    xb = t;
  }
  BigNumber gamma2 = _alpha * xa.pow(2) + _beta * xb;
  gamma_computed = true;
  MQTT.publish(TOPICO_GAMMA1, gamma2.toString());
  BigNumber session_key = compute_session_key(_gamma, xb);
  MQTT.publish(SESSION_KEY_BOB, session_key.toString());
}

void compute_gamma(){
  BigNumber _alpha         = (char*)alpha.c_str();
  BigNumber _beta          = (char*)beta.c_str();
  BigNumber _delta         = (char*)delta.c_str();
  BigNumber _totient_delta = (char*)totient_delta.c_str();
  BigNumber _y             = (char*)y.c_str();
  Serial.print("Y: ");
  Serial.println(_y);
  Serial.print("Alpha: ");
  Serial.println(_alpha);
  Serial.print("Beta: ");
  Serial.println(_beta);
  Serial.print("Delta: ");
  Serial.println(_delta);
  Serial.print("Totient delta: ");
  Serial.println(_totient_delta);

  char r[100] = "";
  randomCharArray((char*)delta.c_str(), r);
  BigNumber xa = r;
  char s[100] = "";
  randomCharArray((char*)totient_delta.c_str(), s);
  BigNumber xb = s;
  xb1 = xb.toString();

  while(xb*_beta % _totient_delta == 0){
    char t[100] = "";
    randomCharArray((char*)totient_delta.c_str(), t);
    xb = t;
  }
  BigNumber _gamma = _alpha * xa.pow(2) + _beta * xb;
  gamma1 = _gamma.toString();
  gamma_computed = true;
  Serial.println();
  Serial.println(String(String("\n- Gamma generated by ") + ID_MQTT + String(".")));
  Serial.println();
  Serial.print("Gamma: ");
  Serial.println(_gamma);
}

//Função: função de callback
//        esta função é chamada toda vez que uma informação de
//        um dos tópicos subescritos chega)
//Parâmetros: nenhum
//Retorno: nenhum
void mqtt_callback(char* topic, byte* payload, unsigned int length)
{
    String msg, _topic;

    //obtem a string do topico recebido
    for(int i = 0; topic[i] != '\0'; i++)
    {
       char c = (char)topic[i];
       _topic += c;
    }
        //obtem a string do payload recebido
    for(int i = 0; i < length; i++)
    {
       char c = (char)payload[i];
       if(_topic.equals(TOPICO_NUSERS)){
         if(isDigit(c)){
            msg += c;
         }
       }else msg += c;
    }
    //toma ação dependendo da string recebida:
    if(_topic.equals(TOPICO_COMMAND) && StrContains(msg.c_str(), ID_MQTT)){
      if(StrContains(msg.c_str(), "disconnect")){
        MQTT.publish(TOPICO_DISCONNECT, ID_MQTT);
        Serial.println("- Disconectado da rede.");
        on_net = false;
      }
    }

    if(_topic.equals(TOPICO_NUSERS)){
      n_users = msg.toInt();
      if(server_id == 0)server_id = n_users;
      Serial.println(server_id);
    }

    if(!gamma_computed){
      if(y.length() == 0 && _topic.equals(TOPICO_Y)){
        y = msg.c_str();
        Serial.println("y recebido.");
      }
      if(alpha.length() == 0 && _topic.equals(TOPICO_ALPHA)){
        alpha = msg.c_str();
        Serial.println("alpha recebido.");
      }
      if(beta.length() == 0 && _topic.equals(TOPICO_BETA)){
        beta = msg.c_str();
        Serial.println("beta recebido.");
      }
      if(delta.length() == 0 && _topic.equals(TOPICO_DELTA)){
        delta = msg.c_str();
        Serial.println("delta recebido.");
      }
      if(totient_delta.length() == 0 && _topic.equals(TOPICO_TOTIENT)){
        totient_delta = msg.c_str();
        Serial.println("totient recebido.");
      }
      if(gamma1.length() == 0 && _topic.equals(TOPICO_GAMMA)){
        Serial.println("gamma recebido.");
        gamma1 = msg.c_str();
      }
    }


    if(!session_key_computed && gamma_computed && server_id == 1){
      if(_topic.equals(TOPICO_GAMMA1)){
        gamma2 = String(msg);
        Serial.println("gamma2 recebido.");
      }
      if(!session_key_computed && y.length() > 0 && gamma2.length() > 0 && xb1.length() > 0 && delta.length() > 0){
        BigNumber _gamma2 = (char*)gamma2.c_str();
        BigNumber _xb1 = (char*)xb1.c_str();

        BigNumber session_key = compute_session_key(_gamma2, _xb1);
        MQTT.publish(SESSION_KEY_ALICE, session_key.toString());
      }
    }

    if(server_id == 1 && !gamma_computed && alpha.length() > 0 && beta.length() > 0 && delta.length() > 0 && totient_delta.length() > 0){
        Serial.println("- Session parameters received.");
        params_received = true;
        compute_gamma();
     }
     if(server_id > 1 && !session_key_computed && y.length() > 0 && gamma1.length() > 0 && delta.length() > 0 && alpha.length() > 0 && totient_delta.length() > 0){
        Serial.println("- Gamma and delta parameters received.");
        compute_gamma1();
    }
    delay(100);
}

//Função: reconecta-se ao broker MQTT (caso ainda não esteja conectado ou em caso de a conexão cair)
//        em caso de sucesso na conexão ou reconexão, o subscribe dos tópicos é refeito.
//Parâmetros: nenhum
//Retorno: nenhum
void reconnectMQTT(){
    while (!MQTT.connected())
    {
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect(ID_MQTT))
        {
            Serial.println("Conectado com sucesso ao broker MQTT!");
            MQTT.subscribe(TOPICO_COMMAND);
            MQTT.subscribe(TOPICO_ALPHA);
            MQTT.subscribe(TOPICO_BETA);
            MQTT.subscribe(TOPICO_DELTA);
            MQTT.subscribe(TOPICO_TOTIENT);
            MQTT.subscribe(TOPICO_NUSERS);
            MQTT.subscribe(TOPICO_GAMMA);
            MQTT.subscribe(TOPICO_GAMMA1);
            MQTT.subscribe(TOPICO_Y);
            MQTT.publish(TOPICO_CONNECT, ID_MQTT);
            on_net = true;
        }else{
            Serial.println("Falha ao reconectar no broker.");
            Serial.println("Havera nova tentatica de conexao em 2s");
            delay(2000);
        }
    }
}

//Função: reconecta-se ao WiFi
//Parâmetros: nenhum
//Retorno: nenhum
void reconectWiFi()
{
    //se já está conectado a rede WI-FI, nada é feito.
    //Caso contrário, são efetuadas tentativas de conexão
    if (WiFi.status() == WL_CONNECTED)
        return;

    WiFi.begin(SSID, PASSWORD); // Conecta na rede WI-FI

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(100);
        Serial.print(".");
    }

    Serial.println();
    Serial.print("Conectado com sucesso na rede ");
    Serial.print(SSID);
    Serial.println("IP obtido: ");
    Serial.println(WiFi.localIP());
}

//Função: verifica o estado das conexões WiFI e ao broker MQTT.
//        Em caso de desconexão (qualquer uma das duas), a conexão
//        é refeita.
//Parâmetros: nenhum
//Retorno: nenhum
void VerificaConexoesWiFIEMQTT(void)
{
    if (!MQTT.connected())
        reconnectMQTT(); //se não há conexão com o Broker, a conexão é refeita

     reconectWiFi(); //se não há conexão com o WiFI, a conexão é refeita
}

//programa principal
void loop()
{
    //keep-alive da comunicação com broker MQTT
    if(on_net){
      if(n_users > 1){
        if(gamma_computed && server_id == 1){
          MQTT.publish(TOPICO_GAMMA, (char*)gamma1.c_str());
          MQTT.publish(TOPICO_DELTA, (char*)delta.c_str());
        }
      }
      //garante funcionamento das conexões WiFi e ao broker MQTT
      VerificaConexoesWiFIEMQTT();
      MQTT.loop();
    }else if(MQTT.connected()){
      while(MQTT.connected()){
        MQTT.disconnect();
        on_net = false;
      }
    }
    delay(500);
}
