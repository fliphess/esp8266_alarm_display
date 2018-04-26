import datetime

import yaml
from yaml.parser import ParserError
from yaml.scanner import ScannerError


class ConfigError(NotImplementedError):
    pass


class Token(dict):
    REQUIRED_FIELDS = (
        "name",
        "token_uid",
        "alarmcode",
    )

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self._verify()

    def _verify(self):
        for key in self.REQUIRED_FIELDS:
            if key not in self.keys() or self[key] == "":
                raise ConfigError('Key {} not found in config for token {}'.format(key, self["name"]))

    def _timeslot(self):
        if "timeslot" not in self or self["timeslot"] == "always" or self["timeslot"] == "":
            return '00:00|23:59'
        return self["timeslot"]

    def _dateslot(self):
        if "dateslot" not in self or self["dateslot"] == "always" or self["dateslot"] == "":
            return datetime.datetime.now().strftime("%d-%m-%Y|%d-%m-%Y")
        return self["dateslot"]

    @staticmethod
    def _strftime(dateslot, timeslot, element):
        timestring = "{} {}".format(timeslot.split('|')[element], dateslot.split('|')[element])
        return datetime.datetime.strptime(timestring, "%H:%M %d-%m-%Y")

    def begin_time(self):
        return self._strftime(self._dateslot(), self._timeslot(), 0)

    def end_time(self):
        return self._strftime(self._dateslot(), self._timeslot(), 1)

    def is_valid(self, code):
        try:
            return code == self["alarmcode"] and self.begin_time() <= datetime.datetime.now() <= self.end_time()
        except ValueError:
            return False


class Settings(object):
    REQUIRED_FIELDS = (
        "mqtt_host",
        "mqtt_port",
        "mqtt_ssl",
        "state_topic",
        "command_topic",
        "rfid_auth_topic",
        "display_topic",
        "tokens",
        "actions"
    )

    def __init__(self, filename):
        self.filename = filename
        self.config = self._read(filename)
        self._parse()
        self._verify()

    @staticmethod
    def _read(filename):
        try:
            with open(filename) as fh:
                return yaml.safe_load(fh.read())
        except (IOError, ParserError, ScannerError) as e:
            raise ConfigError(e)

    def _parse(self):
        self.items = []
        for key, value in self.config.items():
            if not hasattr(self, key):
                self.items.append(key)
                if isinstance(value, str):
                    value = value.format(**self.config)
                setattr(self, key, value)

    def _verify(self):
        for key in self.REQUIRED_FIELDS:
            if key not in self.config.keys():
                raise ConfigError('{} not found in {}'.format(key, self.filename))

    def reload(self):
        self.__init__(filename=self.filename)

    def get_all_tokens(self):
        return [Token(**i) for i in self.tokens]

    def filter_tokens(self, key, value):
        for token in self.get_all_tokens():
            if key in token and token[key] == value:
                return token

    def get_token_by_name(self, name):
        return self.filter_tokens(key='name', value=name)

    def get_token_by_uid(self, uid):
        return self.filter_tokens(key='token_uid', value=uid)
