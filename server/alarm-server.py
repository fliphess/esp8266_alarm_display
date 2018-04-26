#!/usr/bin/env python3
import argparse
import os
import sys

from pid import PidFile, PidFileAlreadyLockedError

from mqtt_listener import get_logger
from mqtt_listener.config import Settings, ConfigError
from mqtt_listener.mqtt import AlarmMQTTListener


def parse_arguments():
    parser = argparse.ArgumentParser(description="Start Alarm MQTT Listener")
    parser.add_argument("-c", "--config", dest="config", type=str, required=True, help="The config file to use")
    parser.add_argument("-v", "--verbosity", action="count", help="increase output verbosity")
    parser.add_argument("-l", "--log", dest="log_file", default='/tmp/mqtt_alarm.log', help="The file to log to")

    args = parser.parse_args()

    if not os.path.isfile(args.config):
        parser.error('Config file {} not found!'.format(args.config))
    return args


def main():
    arguments = parse_arguments()
    logger = get_logger(verbosity=arguments.verbosity, log_file=arguments.log_file)

    try:
        with PidFile('mqtt_listener', piddir='/var/tmp'):
            logger.info("Reading configuration file {}".format(arguments.config))
            settings = Settings(filename=arguments.config)

            logger.info("Starting MQTT Listener server")
            mqtt_server = AlarmMQTTListener(settings=settings, logger=logger)
            mqtt_server.run()
    except ConfigError as e:
        logger.error("Config file contains errors: {}".format(e))
        sys.exit(1)
    except PidFileAlreadyLockedError:
        logger.error("Another instance of this service is already running")
        sys.exit(1)


if __name__ == "__main__":
    main()
