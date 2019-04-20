#deprecated by mouseoutputstack.py #

from config import tap_timings
from sendusbevents import SendUsbEvents

def check_single_tap_movement(cts):
    cts2 = cts.last_touchpad_state
    if (cts2 != None) \
       and (cts.number_of_active_touch == 1) \
       and (cts2.number_of_active_touch == 1) \
       and (cts.tap_chain[0][0] == 1):
        time_delta = 1000*(cts.timestamp - cts.tap_chain[0][1])
        if time_delta > tap_timings[1]:
            fp1 = next(fp for fp in cts.slots if fp.touch==1)
            fp2 = next(fp for fp in cts2.slots if fp.touch==1)
            delx = fp1.abs_x - fp2.abs_x
            dely = fp1.abs_y - fp2.abs_y
            #return {'type':'single_movement','x':delx,'y':dely}
            return SendUsbEvents.move_mouse_queue_object(delx,dely)
            #print("Single Tap Movement:X %d,Y %d"%(delx,dely))
    #Make sure that the number of touches is the same as it was

    


    
