import evdev

devices = [evdev.InputDevice(path) for path in evdev.list_devices()]

for i in devices:
    print(i)

print("Selecting device here..")

keyboard = evdev.InputDevice('/dev/input/event1')
print(keyboard.name)

keyboard.grab()

for event in keyboard.read_loop():
    print (evdev.categorize(event))

