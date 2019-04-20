import serial
import binascii

class SendUsbEvents(object):
    def __init__(self,port='/dev/ttyACM0',speed=1000000,dummy=True):
        #Update this list as more functions are added
        self.handler_list = [ \
                              'send_as_mouse', \
                              'tap_click', \
                              'move_mouse', \
                              'scroll_mouse', \
                              'three_mouse', \
                              'four_mouse', \
                              'five_mouse', \
                              'press_button',\
                              'release_button']
        if dummy:
            self.ser = open('serial.dummy','wb')
        else:
            self.ser = serial.Serial(port,speed)

        self.handler_dict = {}
        self.create_handler_dict()

    def create_handler_dict(self):
        for i in self.handler_list:
            self.handler_dict[i] = getattr(self,i)

    def do(self,call_name,*args):
        print(self.handler_dict)
        print(call_name)
        self.handler_dict[call_name](*args)
            
    def send_as_mouse(self,arr):
        arr2 = [ord('a')]+arr+[ord('\n')]
        #print(binascii.hexlify(bytes(arr2)))
        self.ser.write(bytes(arr2))

    def send_as_midi(self,arr):
        arr2 = [ord('c')]+arr+[ord('\n')]
        #print(binascii.hexlify(bytes(arr2)))
        self.ser.write(bytes(arr2))

    def send_as_keyboard(self,arr):
        arr2 = [ord('b')]+arr+[ord('\n')]
        self.ser.write(bytes(arr2))

    def keyboard(self,key,press_release):
        arr = [key,press_release]
        self.send_as_keyboard(arr)

    def keyboard_press(self,key):
        arr = [key,1]
        self.send_as_keyboard(arr)

    def keyboard_release(self,key):
        arr = [key,0]
        self.send_as_keyboard(arr)
        
    def tap_click(self,num):
        tap_byte = 0b00000001 << (num-1)
        tap_byte |= 0b10000000
        arr = [tap_byte,0,0,0,0]
        self.send_as_mouse(arr)

    def button_state(self,tap_byte):
        arr = [tap_byte,0,0,0,0]
        self.send_as_mouse(arr)
        
    def move_mouse(self,x,y):
        if x<0:
            x=256-abs(x)
        if y<0:
            y=256-abs(y)
        arr = [0,x,y,0,0]
        self.send_as_mouse(arr)
        
    def scroll_mouse(self,x,y):
        if x<0:
            x=256-abs(x)
        if y<0:
            y=256-abs(y)
        arr = [0,0,0,x,y]
        self.send_as_mouse(arr)

    def midi_cc(self,cc_value,value,channel=0):
        #print('midi_cc',cc_value,value)
        if value<0:
            value=127-abs(value)
        assert value<=127, "Midi CC Value exceeds 127"
        assert cc_value <= 127, "MIDi CC Control Number exceeds 127"
        status_byte = 0b10110000 | channel
        arr = [0b00001011,status_byte,cc_value,value]
        self.send_as_midi(arr)

    def midi_note_off(self,note_number,velocity,channel=0):
        assert velocity<=127, "Midi Velocity exceeds 127"
        assert note_number <= 127, "MIDi Note Number exceeds 127"
        status_byte = 0b10000000 | channel
        arr = [0b00001000,status_byte,note_number,velocity]
        self.send_as_midi(arr)

    def midi_note_on(self,note_number,velocity,channel=0):
        assert velocity<=127, "Midi Velocity exceeds 127"
        assert note_number <= 127, "MIDi Note Number exceeds 127"
        status_byte = 0b10010000 | channel
        arr = [0b00001001,status_byte,note_number,velocity,]
        self.send_as_midi(arr)

    def three_mouse(self):
        pass

    def four_mouse(self):
        pass

    def five_mouse(self):
        pass

    def press_button(self,num):
        pass

    def release_button(self,num):
        pass
    
    def disconnect(self):
        self.ser.close()

        
if __name__ == '__main__':
    a = SendUsbEvents(dummy=False)
    #a.tap_click(3)
    a.move_mouse(100,100)
    a.keyboard_press(97)
    a.keyboard_release(97)
    #a.disconnect()
        

    
