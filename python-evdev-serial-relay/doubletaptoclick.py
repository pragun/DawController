#Notes:
#ABBREVIATIONS Used:
# cts : 'Current TouchPad State

from enum import Enum
from fingerposition import FingerPosition
from touchpadstate import TouchPadState

class DoubleTapToClickStage(Enum):
    WAITING = 0
    #Waiting for the first touch on the touchpad,
    #To be in this state means there are no touches on the touchpad

    WAITING2 = 1
    
    PRIMED = 1
    #Waiting for the user to release finger, which would activate this gesture

    ACTIVE = 2
    #This gesture has been activated, this should emit the mouse click event
    #and then immediately go to expired
    #This state never happens coz, for tap to click there is no holding
    #ACTIVE stage, it just emits the mouse event and goes back to waiting
    
    EXPIRED = 3
    # Finger rested too long,
    # or there are too many fingers,
    # or the finger moved too much
    # We're gonna have to wait until the cts is empty to go to WAITING


#Just a pretty alias for use internally in this file
Stage = DoubleTapToClickStage

max_movement_radius = 100
max_finger_rest_time = 0.100

class TapToClick(object):
    def __init__(self):
        self.tp_primed_state = None
        self.active_slot_number = None
        
        self.state_checkfunc = {
            Stage.WAITING: self.check_waiting,
            Stage.PRIMED: self.check_primed,
            Stage.ACTIVE: self.check_active,
            Stage.EXPIRED: self.check_expired,
            }

        self.life_state = Stage.WAITING

    def check_state(self,cts):
        print("Previous state: ",self.life_state)
        a = self.state_checkfunc[self.life_state](cts)
        print("Post state: ",self.life_state)
        return a
    
    def check_waiting(self,cts):
        #If there is already more than one fingers on the touchpad
        if cts.number_of_active_touch == 0:
            self.life_state = Stage.WAITING
            return False

        if cts.number_of_active_touch == 1:
            self.life_state = Stage.PRIMED
            self.tp_primed_state = TouchPadState.copy(cts)
            self.active_slot_number = [i.touch for i in cts.slots].index(1)
            return False

        else:
            self.life_state = Stage.EXPIRED
            return False
            
    def check_primed(self,cts):
        so_far_ok = True
        
        #If there has been more than one tap
        if cts.number_of_active_touch > 1:
            self.life_state = Stage.EXPIRED
            return False 
        
        #If there has been a lot of movement
        fp_orig = self.tp_primed_state.slots[self.active_slot_number]
        fp_now = cts.slots[self.active_slot_number]
        
        distance_moved = fp_orig.distance(fp_now)
        print("Distance Moved....%f"%(distance_moved,))
        if distance_moved > max_movement_radius:
            self.life_state = Stage.EXPIRED
            return False
        
        #If there has been a lot of time since the first tap
        finger_rest_time = cts.timestamp - self.tp_primed_state.timestamp
        print("Finger Rest Time ..%f"%(finger_rest_time,))
        if finger_rest_time > max_finger_rest_time:
            self.life_state = Stage.EXPIRED
            return False

        #If things are as they are
        if cts.number_of_active_touch == 1:
            self.life_state = Stage.PRIMED
            return False

        #Aha, if the finger has been lifted,
        #We'll return true to emit one event, and set this to waiting
        if cts.number_of_active_touch == 0:
            self.life_state = Stage.WAITING
            return True
        
    def check_active(self,cts):
        pass
        
    def check_expired(self,cts):
        #If there is already more than one fingers on the touchpad
        if cts.number_of_active_touch == 0:
            self.life_state = Stage.WAITING
            return False

        else:
            self.life_state = Stage.EXPIRED
            return False
        


if __name__ == '__main__':
    from datetime import datetime
    import time
    #We're gonna check this her

    tps = TouchPadState()
    ttclk = TapToClick()
    
    def timed_moved_check(tps,ttclk,t,dx,dy,extra_touch):
        print("\nBeginning with an empty touchpad")
        print(ttclk.check_state(tps))
        
        print("\nAdding a finger")
        tps.slots[0].touch = 1
        p = datetime.now().timestamp()
        tps.timestamp = p    
        tps.update_num_touches()
        print(ttclk.check_state(tps))
        
        time.sleep(t)
        print("\nChecking again with the finger, after %f seconds"%t)
        print("\nMoving the touch by %d,%d"%(dx,dy))
        tps.slots[0].touch = 1
        tps.slots[0].abs_x += dx
        tps.slots[0].abs_y += dy

        print("Extra touch point: %d",extra_touch)
        if extra_touch == 1:
            tps.slots[1].touch = 1
        p = datetime.now().timestamp()
        tps.timestamp = p
        tps.update_num_touches()
        print(ttclk.check_state(tps))
        
        print("\nRemoving a finger")
        tps.slots[0].touch = 0
        p = datetime.now().timestamp()
        tps.timestamp = p
        tps.update_num_touches()
        print(ttclk.check_state(tps))

    timed_moved_check(tps,ttclk,0.001,0,0,0)
    timed_moved_check(tps,ttclk,0.06,0,0,0)
    timed_moved_check(tps,ttclk,0.001,100,100,0)
    timed_moved_check(tps,ttclk,0.001,00,00,1)
    
