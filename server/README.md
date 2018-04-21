# MQTT Alarm display - Server Component

This is the server side component of the mqtt alarm display.
It listens on a MQTT topic for authentication requests.
If the UID and the passcode match a person in the config and this person is allowed in the current timeslot,
An action will be performed, depending on which user input was performed on the display.

## Install

Create a virtualenv and install requirements

```
  cd server/
  mkvirtualenv -p "$( which python3 )" -a "$( pwd )" mqtt-listener
  pip install -r requirements.txt
```

## Configure

Copy the config file and adjust the settings

```
  cp settings.yaml.example settings.yaml && vim settings.yaml
```

## Run

Run the server:

```
./alarm-server.py -c settings.yaml -vvvv
```
