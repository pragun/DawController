import evdev

devices = [evdev.InputDevice(path) for path in evdev.list_devices()]

for i in devices:
    print(i)

    
