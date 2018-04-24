import json
import os
import socket
import sys
import time
import paho.mqtt.client as mqtt

from json import JSONDecodeError
from mqtt_listener.state import alarm_state


class AlarmMQTTListener(mqtt.Client):
    def __init__(self, settings, logger, **kwargs):
        super().__init__(**kwargs)
        self.settings = settings
        self.logger = logger

    def on_message_state(self, mqttc, obj, msg):
        self.logger.info("Incoming State change! Received state is: {}. Sending forward".format(msg.payload))
        alarm_state.set_state(msg.payload)
        mqttc.publish(topic=self.settings.display_topic, payload=alarm_state.get_state(), retain=True)

    def on_message_rfid(self, mqttc, obj, msg):
        try:
            incoming_data = json.loads(msg.payload.decode('UTF-8'))
        except JSONDecodeError:
            self.logger.error('Failed to decode incoming message: {}'.format(msg.payload))
            return

        for field in ("code", "hostname", "uid", "action"):
            if field not in incoming_data:
                self.logger.error("Incorrect json input: required key \"{}\" not found in payload: {}".format(field, msg.payload))
                return

        self.logger.info("Incoming auth payload! The received data is: \"{}\"".format(incoming_data))
        self._process_data(incoming_data, mqttc)

    def on_connect(self, mqttc, obj, flags, rc):
        self.logger.info("Notifying broker we're online")
        mqttc.publish(topic="hass/status", payload="Alarm MQTT Listener Online")

        self.logger.info("Connected to MQTT broker, subscribing to topics...")
        for topic in self.settings.state_topic, self.settings.rfid_auth_topic:
            mqttc.subscribe(topic)

    def on_disconnect(self, mqttc, obj, rc):
        self.logger.error("Connection to broker failed... Reconnecting.")

    def on_subscribe(self, mqttc, obj, mid, granted_qos):
        self.logger.info("Subscribed to: {} {}".format(str(mid), str(granted_qos)))

    def on_log(self, mqttc, obj, level, string):
        self.logger.debug(string)

    def _process_data(self, incoming_data, mqttc):
        response_topic = "{}/{}".format(self.settings.display_topic, incoming_data["hostname"])

        token = self.settings.get_token_by_uid(uid=incoming_data["uid"])
        name = token["name"] if token and "name" in token else "unknown"

        if token and token.is_valid(incoming_data["code"]):
            self.logger.warn("Accepted token from {}, uid is: \"{}\"".format(token["name"], incoming_data["uid"]))
            mqttc.publish(
                topic=response_topic,
                payload=json.dumps(dict(access="GRANTED", uid=token["token_uid"], name=token["name"]))
            )
            self._send_command(mqttc, incoming_data["action"])

        else:
            self.logger.warn("Denied alarmcode from {}, uid is: \"{}\"".format(name, incoming_data["uid"]))
            mqttc.publish(
                topic=response_topic,
                payload=json.dumps(dict(access="DENIED", uid=incoming_data["uid"], name=name))
            )

    def _send_command(self, mqttc, action):
        payload = self.settings.actions.get(action, None)
        if payload:
            time.sleep(5)
            mqttc.publish(topic=self.settings.command_topic, payload=payload)

    def _setup_listener(self):
        self.message_callback_add(self.settings.state_topic, self.on_message_state)
        self.message_callback_add(self.settings.rfid_auth_topic, self.on_message_rfid)
        self.reconnect_delay_set(min_delay=1, max_delay=30)

        if self.settings.mqtt_username and self.settings.mqtt_password:
            self.username_pw_set(username=self.settings.mqtt_username, password=self.settings.mqtt_password)

        if self.settings.mqtt_ssl and (self.settings.mqtt_ca_certs and os.path.isfile(self.settings.mqtt_ca_certs)):
            self.tls_set(ca_certs=self.settings.mqtt_ca_certs)

        self.logger.info("Connecting to mqtt broker: {}://{}:{}".format(
            "mqtt/ssl" if self.settings.mqtt_ssl else "mqtt",
            self.settings.mqtt_host,
            self.settings.mqtt_port)
        )
        self.connect(host=self.settings.mqtt_host, port=self.settings.mqtt_port, keepalive=60)

    def run(self):
        self._setup_listener()

        while True:
            try:
                self.loop_forever()
            except socket.error:
                time.sleep(5)

            except KeyboardInterrupt:
                self.logger.warning("Ctrl+C Pressed! Quitting Listener.")
                sys.exit(1)
