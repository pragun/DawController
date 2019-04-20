#deprecated by mouseoutputstack.py #


from config import tap_timings
from sendusbevents import SendUsbEvents

def FourMouse(a):
    print("FourMouse")
    
def ThreeMouse(a):
    print("ThreeMouse")

def FiveMouse(a):
    print("FiveMouse")
    
def check_multi_movement(cts):
    a = check_n_tap_movement(cts)
    if a is not None:
        (n,delx,dely) = a
        print(a)
        if n == 1:
            return ('move_mouse',int(delx),int(dely))
        if n == 2:
            return ('scroll_mouse',int(delx),int(dely))
        if n == 3:
            return ('three_mouse',)
        if n == 4:
            return ('four_mouse',)
        if n == 5:
            return ('five_mouse',)
        
def check_n_tap_movement(cts):
    cts2 = cts.last_touchpad_state
    n = cts.number_of_active_touch
    if (n > 0) \
       and (cts2 != None) \
       and (cts2.number_of_active_touch == n) \
       and (cts.tap_chain[0][0] == n):
        time_delta = 1000*(cts.timestamp - cts.tap_chain[0][1])
        if time_delta > tap_timings[n]:
            delx = cts.centroid_x - cts2.centroid_x
            dely = cts.centroid_y - cts2.centroid_y
            return (n,delx,dely)
    #Make sure that the number of touches is the same as it was
