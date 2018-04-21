
class Singleton(type):
    def __init__(cls, name, bases, attrs, **kwargs):
        super().__init__(name, bases, attrs)
        cls._instance = None

    def __call__(cls, *args, **kwargs):
        if cls._instance is None:
            cls._instance = super().__call__(*args, **kwargs)
        return cls._instance


class AlarmState(metaclass=Singleton):
    def __init__(self, state="unknown"):
        self.state = state

    def set_state(self, state):
        self.state = state.decode('unicode_escape')

    def get_state(self):
        return self.state

    def __repr__(self):
        return self.state


alarm_state = AlarmState()
