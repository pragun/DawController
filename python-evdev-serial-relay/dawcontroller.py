#This here takes care of 
# 1. listening to touchpad events
# 2. Updating touchpad state
# 3. Runinng gesture recognition 
# 4. And emitting sendMouse events

import evdev, asyncio, os
import datetime
from evdev_device_mappings import *
from sendusbevents import SendUsbEvents
from mouseoutputstack import MouseOutputStack
from collections import defaultdict
key_config_filename = 'key_config'
from tabulate import tabulate
import math
import arduino_keys
import datetime
from config import movement_debounce_ms


now_timestamp_f = lambda : datetime.datetime.now().timestamp()

sign = lambda x: (1, -1)[x<0]

class MouseEventHandler(object):
    def __init__(self,sendUsbEvents):
        super().__init__()
        
        #The following variables hold state variables
        #for understanding sequential events
        self.sendUsbEvents = sendUsbEvents
        self.rel_x_value = 0
        self.rel_y_value = 0
        self.rel_wheel_value = 0
        self.btn_mask_byte = 0
        self.btn_flag = 0
        self.rel_movement_flag = 0
        self.rel_wheel_flag = 0
        
        self.pressed_key = None

        #Reporting general counters and state variables.Why?
        #To handle the fact that some events such, as sending a keystroke
        #when a key on the mouse is pressed, only make sense if there was no movement
        #while the key was pressed. Coz, if there was movement, the movement might
        #have its own events
        #This is especially true for key_press_release, with the if_no_movement Flag.

        #However, inevitable there might be a slight amount of movement
        #if I move the mouse as I press/release the key. To avoid that slight movement
        #this pressed_key_report_state saves the timestamp of the last keychange
        #any motion is ignored untill the debouncing period
        self.timestamp_of_last_keychange = now_timestamp_f()
        self.leftover_movement_report_data = None
        self.number_movement_reports_since_last_key_change = 0
        #End of Reporting general counters

        #This Flag is set when there is a change in the number of tracking ids        
        self.handler_dict_abs = {
        }

        self.handler_dict_syn = {
            evdev.ecodes.SYN_REPORT : self.syn_report,
        }

        self.handler_dict_key = {
            evdev.ecodes.BTN_LEFT : self.btn_left,
            evdev.ecodes.BTN_RIGHT : self.btn_right,
            evdev.ecodes.BTN_MIDDLE : self.btn_middle,
            evdev.ecodes.BTN_EXTRA : self.btn_extra,
        }

        self.handler_dict_msc = defaultdict(lambda:self.do_nothing)
        

        self.handler_dict_rel = {
            evdev.ecodes.REL_X : self.rel_x,
            evdev.ecodes.REL_Y : self.rel_y,
            evdev.ecodes.REL_WHEEL : self.rel_wheel,
        }

        self.handler_dict = {
            evdev.ecodes.EV_SYN : self.handler_dict_syn,
            evdev.ecodes.EV_KEY : self.handler_dict_key,
            evdev.ecodes.EV_ABS : self.handler_dict_abs,
            evdev.ecodes.EV_REL : self.handler_dict_rel,
            evdev.ecodes.EV_MSC : self.handler_dict_msc,
        }

        self.key_dict_movement = {}
        self.key_dict_press_release = {}
        self.key_dict_wheel = {}
        self.read_key_config()
        

    def read_key_config(self):
        key_config_token_list = None
        with open(key_config_filename,'r',newline='') as f:
            output = []
            for line in f:
                if line.startswith('#'):
                    continue #Ignore comments, like this one here :)
                output.append(line.split())
            output = [i for i in output if i != []]
            key_config_token_list = output

        for line in key_config_token_list:
            print(line)
            [key_dict_key,movement_handler,wheel_handler,press_release_handler] = line
            key_dict_key = eval(key_dict_key)
            self.key_dict_movement[key_dict_key] = eval('self.'+movement_handler)
            self.key_dict_wheel[key_dict_key] = eval('self.'+wheel_handler)
            self.key_dict_press_release[key_dict_key] = eval('self.'+press_release_handler)

        print("Loaded Key Configuration Successfully.")
        print(tabulate(output))


#All of these here, till #end_of_config functions
#can be used in the key_config file to denote
#what should happen if the keys are pressed and released
#and if the mouse is moved when the keys are pressed
    def note_on_off(self,note):
        def helperfunction(key_up_down_value):                           
            if key_up_down_value == 1:
                self.sendUsbEvents.midi_note_on(note,127)
            if key_up_down_value == 0:
                self.sendUsbEvents.midi_note_off(note,127)
        return helperfunction                
    
    def std_cc_xy(self):
        def helperfunction(x,y):
            now = now_timestamp_f()
            del_ms_since_key_change = 1000*(now - self.timestamp_of_last_key_change)
            #print("Debounce timer:%f"%(del_ms_since_key_change,))
            if del_ms_since_key_change < movement_debounce_ms:
                return

            last_state = self.leftover_movement_report_data
            last_state = (0,0) if last_state == None else last_state
            #[total delX, delY since beginning of gesture]
            
            x = min(x,63) if x>0 else max(x,-63)
            y = min(y,63) if y>0 else max(y,-63)
            self.sendUsbEvents.midi_cc(1,x)
            self.sendUsbEvents.midi_cc(2,y)
            (rx,ry) = last_state
            new_rx = rx+x
            new_ry = ry+y
            del_r = int(math.sqrt(new_rx**2+new_ry**2)-math.sqrt(rx**2+ry**2))
            self.sendUsbEvents.midi_cc(3,del_r)
            del_d = int(math.sqrt(x**2+y**2))
            self.sendUsbEvents.midi_cc(4,del_d)
            self.leftover_movement_report_data = (new_rx,new_ry)
            
        return helperfunction

    
    def report_xy_as_scroll(self,divisor):
        def helperfunction(x,y):
            now = now_timestamp_f()
            del_ms_since_key_change = 1000*(now - self.timestamp_of_last_key_change)
            #print("Debounce timer:%f"%(del_ms_since_key_change,))
            if del_ms_since_key_change < movement_debounce_ms:
                return

            last_state = self.leftover_movement_report_data
            last_state = (0,0) if last_state == None else last_state
            #[Remaining movement amounts after removing the divisor]

            (x_remainder,y_remainder) = last_state
            x_total = x+x_remainder
            y_total = y+y_remainder
            x_remainder = sign(x_total)*(abs(x_total)%divisor)
            y_remainder = sign(y_total)*(abs(y_total)%divisor)
            x_report = int(x_total/divisor)
            y_report = int(y_total/divisor)

            #print("X:%d,Y:%d XT:%d,YT:%d XRem:%d,YRem:%d XR:%d,YR:%d\n"\
            #      %(x,y,x_total,y_total,x_remainder,y_remainder,x_report,y_report))

            if (abs(x_report) > 0) or (abs(y_report) > 0):
                self.sendUsbEvents.scroll_mouse((-1*y_report),x_report)
                self.number_movement_reports_since_last_key_change += 1
                
            self.leftover_movement_report_data = (x_remainder,y_remainder)
            
        return helperfunction

    def report_xy_as_cursor(self,divisor):
        def helperfunction(x,y):
            now = now_timestamp_f()
            del_ms_since_key_change = 1000*(now -self.timestamp_of_last_key_change)
            #print("Debounce timer:%f"%(del_ms_since_key_change,))

            if del_ms_since_key_change < movement_debounce_ms:
                return
            
            last_state = self.leftover_movement_report_data
            last_state = (0,0) if last_state == None else last_state
            #[Remaining movement amounts after removing the divisor]
            
            (x_remainder,y_remainder) = last_state
            x_total = x+x_remainder
            y_total = y+y_remainder

            #print('X:%d, Y:%d, Xt:%d,Yt:%d, Xr:%d, Yr:%d'%(x,y,x_total,y_total,x_remainder,y_remainder))
            
            x_remainder = sign(x_total)*(abs(x_total)%divisor)
            y_remainder = sign(y_total)*(abs(y_total)%divisor)
            x_quot = int(x_total/divisor)
            y_quot = int(y_total/divisor)

            new_state = (x_remainder,y_remainder)
            report_sent = False

            if x_quot >= 1:
                self.sendUsbEvents.keyboard_press(arduino_keys.KEY_RIGHT_ARROW)
                self.sendUsbEvents.keyboard_release(arduino_keys.KEY_RIGHT_ARROW)
                new_state = (x_remainder,0)
                report_sent = True
            
            if x_quot <= -1:
                self.sendUsbEvents.keyboard_press(arduino_keys.KEY_LEFT_ARROW)
                self.sendUsbEvents.keyboard_release(arduino_keys.KEY_LEFT_ARROW)
                new_state = (x_remainder,0)
                report_sent = True

            if y_quot >= 1:
                self.sendUsbEvents.keyboard_press(arduino_keys.KEY_DOWN_ARROW)
                self.sendUsbEvents.keyboard_release(arduino_keys.KEY_DOWN_ARROW)
                new_state = (0,y_remainder)
                report_sent = True
                
            if y_quot <= -1:
                self.sendUsbEvents.keyboard_press(arduino_keys.KEY_UP_ARROW)
                self.sendUsbEvents.keyboard_release(arduino_keys.KEY_UP_ARROW)            
                new_state = (0,y_remainder)
                report_sent = True
                               
            self.leftover_movement_report_data = new_state
            if report_sent:
                self.number_movement_reports_since_last_key_change += 1
                
            #print('X:%d,Y:%d,R:%f'%(x,y,x/float(y)))
        return helperfunction

    def key_press_release(self,key_code):
        def helperfunction(key_up_down_value):
            if key_up_down_value == 0: #This means that the key is being released
                if self.number_movement_reports_since_last_key_change == 0:
                    self.sendUsbEvents.keyboard_press(key_code)
                    self.sendUsbEvents.keyboard_release(key_code)
        return helperfunction
    
    def cc_whl(self,cc_value):
        def helperfunction(whl):
            self.sendUsbEvents.midi_cc(cc_value,whl)
        return helperfunction

    def report_movement(self,factor):
        def helperfunction(x,y):
            #print('RM x:%d, y:%d'%(x,y))
            self.sendUsbEvents.move_mouse(int(x/factor),int(y/factor))            
        return helperfunction

    def report_scroll(self):
        def helperfunction(whl):
            self.sendUsbEvents.scroll_mouse(whl,0)
        return helperfunction
    
    def do_nothing(self,*args): #Wu-Wei
        def helperfunction(*args):
            pass
        return helperfunction
#end_of_key_config_functions

    def process_mouse_event(self,event):
        try:
            self.handler_dict[event.type][event.code](event)
        except Exception as e:
            print("Exception:",e) 
            print("Event Categorize:",evdev.categorize(event))
            print(event)

    def process_keyboard_event(self,event):
        if event.type == 1:
            #print("Event Cat:",evdev.categorize(event))
            #print(event)

            def reset_counters():
                #Reset counters
                self.timestamp_of_last_key_change = now_timestamp_f()
                self.number_movement_reports_since_last_key_change = 0
                self.leftover_movement_report_data = None

            if event.value == 1: #Key Down
                reset_counters()
                self.pressed_key = event.code
                self.key_dict_press_release[self.pressed_key](event.value)
                
            if event.value == 0: #Key Up
                self.key_dict_press_release[self.pressed_key](event.value)
                self.pressed_key = None
                reset_counters()
                
            
    #This is the event that ties everything together
    #In other words, when this is received we
    # 1. Update the trackpad state with all the information we've received
    # 2. See if that requires us to send out any mouse events
    # 3. Prepare for receiving the next SYN event, whenever that will happen

    def rel_x(self,event):
        self.rel_x_value = event.value
        self.rel_movement_flag = True

    def rel_y(self,event):
        self.rel_y_value = event.value
        self.rel_movement_flag = True

    def rel_wheel(self,event):
        self.rel_wheel_value = event.value
        self.rel_wheel_flag = True

    def btn_left(self,event):
        self.btn_flag = True
        self.btn_left_value = event.value
        if(event.value):
            self.btn_mask_byte |= 0x1
        else:
            self.btn_mask_byte &= 0b11111110
        
    def btn_right(self,event):
        self.btn_flag = True
        self.btn_right_value = event.value
        if(event.value):
            self.btn_mask_byte |= 0x2
        else:
            self.btn_mask_byte &= 0b11111101

    def btn_middle(self,event):
        self.btn_flag = True
        self.btn_middle_value = event.value
        if(event.value):
            self.btn_mask_byte |= 0x4
        else:
            self.btn_mask_byte &= 0b11111011

    def btn_extra(self,event):
        print("btn extra",event.value)
        
    def syn_report(self,event):
        if self.btn_flag:
            self.sendUsbEvents.button_state((self.btn_mask_byte|0b10000000))
            self.btn_flag = False

        if self.rel_movement_flag:
            self.key_dict_movement[self.pressed_key](self.rel_x_value,self.rel_y_value)
            self.rel_movement_flag = False
            
        if self.rel_wheel_flag:
            self.key_dict_wheel[self.pressed_key](self.rel_wheel_value)
            self.rel_wheel_flag = False

        self.rel_x_value = 0
        self.rel_y_value = 0
        self.rel_wheel_value = 0

if __name__ == '__main__':
    sendUsbEvents = SendUsbEvents(dummy=False)
    utech_mouse_event_handler = MouseEventHandler(sendUsbEvents)

    mouse = evdev.InputDevice(utech_mouse)
    keyboard = evdev.InputDevice(utech_keyboard)

    mouse.grab()
    keyboard.grab()

    async def mouse_event_loop():
        async for ev in mouse.async_read_loop():
            utech_mouse_event_handler.process_mouse_event(ev)

    async def keyboard_event_loop():
        async for ev in keyboard.async_read_loop():
            utech_mouse_event_handler.process_keyboard_event(ev)
        
    async def main():
        mouse_event_handler_task = asyncio.create_task(mouse_event_loop())
        keyboard_event_handler_task = asyncio.create_task(keyboard_event_loop())
        await asyncio.gather(mouse_event_handler_task, keyboard_event_handler_task)
        
    asyncio.run(main())
