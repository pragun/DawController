#Touchpad State 

from fingerposition import FingerPosition
from collections import deque
from tabulate import tabulate

max_history = 20
max_fingers = 5

# Finger State ID is changed every time
# a TRACKING_ID event is recevied, meaning there is a change
# of the "state of fingers" on the trackpad
# meaning a finger has been lifted/dropped
# this can be used to figure out if a movement
# is a part of a continued movement or if its a brand new
# kinda movement


class TouchPadState(object):
    def __init__(self):
        self.number_of_active_touch = 0
        #Number of active touches at the last syn report
        self.slots = [FingerPosition(i) for i in range(5)]
        self.timestamp = None
        self.last_number_of_fingers = 0
        self.tap_chain = deque([(0,0.0) for i in range(max_history)],maxlen=max_history)
        self.last_touchpad_state = None
        self.centroid_x = None
        self.centroid_y = None
        self.tracking_id_flag = False
        self.movement_flag = False
        self.finger_state_id = int(0)
        #This one here is changed by MT_Handler
        # the child class of TouchPadState
        
        
    def calculate_centroid(self):
        if self.number_of_active_touch == 0:
            self.centroid_x = float('nan')
            self.centroid_y = float('nan')
        else:
            self.centroid_x = float(sum([fp.abs_x for fp in self.slots if fp.touch == 1]))/self.number_of_active_touch
            self.centroid_y = float(sum([fp.abs_y for fp in self.slots if fp.touch == 1]))/self.number_of_active_touch
        
        
    def update_internal_state_syn_report(self):
        self.number_of_active_touch = sum([i.touch for i in self.slots])
        self.update_tap_chain()
        self.calculate_centroid()

        if self.tracking_id_flag: #This is set by the MT Handler
            #This means that the number of fingers
            #on the touchpad has changed
            #Check on all events that depend on this kinda thing
            #Basically all events            
            self.finger_state_id = (self.finger_state_id+1)%10000
            #self.check_and_queue_event(check_for_tap_event(self))
            
        
    def reset_for_next_syn_report(self):
        self.last_touchpad_state = TouchPadState.simple_copy(self)
        self.tracking_id_flag = 0
        self.movement_flag = 0
            
    
    @classmethod
    def simple_copy(cls,i): #i, is the TouchPadState object to create a deepcopy of
        # In most usage, i would be a MT_Handler type class that inherits
        # TouchPadState
        a = cls()
        a.number_of_active_touch = i.number_of_active_touch
        a.slots = [FingerPosition.copy(j) for j in i.slots]
        a.timestamp = i.timestamp
        a.centroid_x = i.centroid_x
        a.centroid_y = i.centroid_y
        a.finger_state_id = i.finger_state_id
        return a

    
    def distance_moved(self,tp_other):
        pass

    
    def update_tap_chain(self):
        if self.last_number_of_fingers != self.number_of_active_touch:
            self.tap_chain.appendleft((self.number_of_active_touch,self.timestamp))
            self.last_number_of_fingers = self.number_of_active_touch
            return True
        else:
            return False

        
    def __repr__(self):
        slot_text = '\n'.join([str(i) for i in self.slots])
        active_touches = 'Active Touches: %d'%(self.number_of_active_touch)
        misc_text = 'FingerStateId:%d Centroid: X:%f, Y:%f'%(self.finger_state_id,self.centroid_x,self.centroid_y)        
        tap_list_text = tabulate(list(self.tap_chain)[:10],floatfmt=".4f")
        return '\n'.join([slot_text,active_touches,misc_text, tap_list_text]) 
