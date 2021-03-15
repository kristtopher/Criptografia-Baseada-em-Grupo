import paho.mqtt.client as mqttClient
import paho.mqtt.subscribe as subscribe
from sympy.ntheory.factor_ import totient
import sympy
import random

import time

params = {
    "delta": 1,
    "alpha": 1,
    "beta":  1,
    "totient_delta": 0,
    "y": 0
}

pn = []
pm = []
psize = 20
n = 3
m = 3

MQTT_USERNAME = ""
MQTT_PASSWORD = ""

CONNECT_USER = "dcc075/users/connect"
DISCONNECT_USER = "dcc075/users/disconnect"
NUMBER_USERS = "dcc075/users/number_users"
COMMAND_USER = "dcc075/users/command"
PARAM_ALPHA = "dcc075/params/alpha"
PARAM_BETA = "dcc075/params/beta"
PARAM_Y = "dcc075/params/y_param"
PARAM_DELTA = "dcc075/params/delta"
PARAM_TOTIENTDELTA = "dcc075/params/totient_delta"
SESSION_KEY_BOB = "dcc075/sessionkey/bob"
SESSION_KEY_ALICE = "dcc075/sessionkey/alice"
N_USERS = 0
USERS = []
sess_param_computed = False


def broadcast_to_users(command):
    print()
    for user in USERS:
        msg = user.decode("utf-8") + "_{}".format(command)
        client.publish(COMMAND_USER, msg)
        print("- Command \'{0}\' sent to \'{1}\'.".format(command, user))


def generate_session_parameters():
    global sess_param_computed
    delta = 1
    alpha = 1
    beta = 1
    temp_q = 1

    for _ in range(m):
        p = sympy.sieve[random.randrange(psize)]
        pm.append(p)
        temp_q *= p

    for _ in range(n):
        p = sympy.sieve[random.randrange(psize)]
        while p in pm:
            p = sympy.sieve[random.randrange(psize)]
        pn.append(p)
        delta *= pow(p, 3)
        alpha *= pow(p, 2)*(p-1)
        beta *= p*(p-1)*temp_q

    y = random.randrange(delta)
    while sympy.igcd(y, delta) != 1:
        y = random.randrange(delta)

    sess_param_computed = True
    params["alpha"] = alpha
    params["delta"] = delta
    params["beta"] = beta
    params["totient_delta"] = totient(delta)
    params["y"] = y


def on_message(client, userdata, message):
    global N_USERS, USERS

    if message.topic == CONNECT_USER:
        if message.payload not in USERS:
            USERS.append(message.payload)
            N_USERS += 1
            client.publish(NUMBER_USERS, N_USERS, qos=2)
            print("\n %s connected." % (message.payload))
            print("Users: {}".format(USERS))
            if sess_param_computed:
                client.publish(PARAM_Y, params["y"], qos=2)
                client.publish(PARAM_ALPHA, params["alpha"], qos=2)
                client.publish(PARAM_DELTA, params["delta"], qos=2)
                client.publish(PARAM_BETA, params["beta"], qos=2)
                client.publish(PARAM_TOTIENTDELTA,
                               params["totient_delta"], qos=2)

            if N_USERS == 1:
                print("\nGenerating session parameters...")
                generate_session_parameters()
                print("Parameters: {}".format(params))
                if sess_param_computed:
                    client.publish(PARAM_Y, params["y"], qos=2)
                    client.publish(PARAM_ALPHA, params["alpha"], qos=2)
                    client.publish(PARAM_DELTA, params["delta"], qos=2)
                    client.publish(PARAM_BETA, params["beta"], qos=2)
                    client.publish(PARAM_TOTIENTDELTA,
                                   params["totient_delta"], qos=2)

        else:
            print("{} is already connected.".format(message.payload))

    if message.topic == "dcc075/sessionkey/bob":
        print("Bob session key: " + message.payload)

    if message.topic == "dcc075/sessionkey/alice":
        print("Alice session key: " + message.payload)

    if message.topic == DISCONNECT_USER:
        if message.payload in USERS:
            USERS = [user for user in USERS if message.payload != user]
            N_USERS -= 1
            print("\n %s disconnected." % (message.payload))
            print("Users: {}".format(USERS))


def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to broker")

        global Connected  # Use global variable
        Connected = True  # Signal connection
    else:

        print("Connection failed")

    client.subscribe(CONNECT_USER, qos=2)
    client.subscribe(DISCONNECT_USER, qos=2)
    client.subscribe(SESSION_KEY_BOB, qos=2)
    client.subscribe(SESSION_KEY_ALICE, qos=2)


Connected = False  # global variable for the state of the connection

broker_address = "broker.hivemq.com"
port = 1883

client = mqttClient.Client("Trevor_!@#!@")  # create new instance
client.on_connect = on_connect  # attach function to callback
client.on_message = on_message
client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
client.connect(broker_address, port=port)  # connect to broker

client.loop_start()  # start the loop

while Connected != True:  # Wait for connection
    time.sleep(0.1)

try:
    while True:
        pass


except KeyboardInterrupt:
    broadcast_to_users("disconnect")
    while len(USERS) > 0:
        pass
    client.disconnect()
    client.loop_stop()
