import logging
import sys


def get_verbosity(verbosity):
    return {
        0: logging.ERROR,
        1: logging.WARN,
        2: logging.INFO,
        3: logging.DEBUG,
        None: logging.WARN,
    }.get(verbosity, logging.DEBUG)


def get_logger(verbosity=2, log_file="/tmp/mqtt_alarm.log"):
    log_format = "[%(asctime)s] %(name)s | %(funcName)-20s | %(levelname)s | %(message)s"
    logging.basicConfig(filename=log_file, level=logging.INFO, filemode="w", format=log_format)
    logger = logging.getLogger('mqtt_alarm_listener')

    stream_handler = logging.StreamHandler(sys.stderr)
    stream_handler.setFormatter(logging.Formatter(fmt=log_format))

    if not logger.handlers:
        logger.setLevel(get_verbosity(verbosity))
        logger.addHandler(stream_handler)

    return logger
