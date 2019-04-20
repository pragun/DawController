import serial

class SendEventsOverUsb(object):
    def __init__(self,port='/dev/ttyACM0',speed=1000000):
        self.ser = serial.Serial(port,speed)

    def send_as_mouse(self,arr):
        arr2 = [ord('a')]+arr+[ord('\n')]
        print(bytes(arr2))
        self.ser.write(bytes(arr2))

    def tap_click(self,num):
        tap_byte = 0b00000001 << (num-1)
        tap_byte |= 0b10000000
        arr = [tap_byte,0,0,0,0]
        self.send_as_mouse(arr)

    def move(self,x,y):
        arr = [0,x,y,0,0]
        self.send_as_mouse(arr)
        
    def disconnect(self):
        self.ser.close()

        
if __name__ == '__main__':
    a = SendEventsOverUsb()
    a.tap_click(3)
    a.move(100,100)
    a.disconnect()
        
