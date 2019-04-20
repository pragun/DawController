#deprecated by mouseoutputstack.py #

#From the chain of taps on the touchpad,
#this will check if the timing indeed matches what
#it needs to be for a single tap, double tap, etc
#after that it will create an event in the event queue

from config import tap_timings
from sendusbevents import SendUsbEvents

def check_for_tap_event(cts):
    if cts.number_of_active_touch == 0:
        max_tap = 0
        end_chain = 0
        list_tap_chain = list(cts.tap_chain)[1:] #Here we take the
        #first element out because, the for loop is going to look for
        #the last time there were no fingers on the touchpad.
        #But since the first element [by the start condition]
        #is also that we remove it so the for loop can find the
        #last time there were no fingers on the touchpad
        
        for i,(n,t) in enumerate(list_tap_chain):
            max_tap = max(max_tap,n)
            if n == 0:
                start_chain = i-1
                #print("Found start of previous event chain at %f, index:%d"%(t,i))
                #print("MaxTap:%d"%max_tap)
                break
            
            
        start_timestamp = list_tap_chain[start_chain][1]
        delta_t = cts.timestamp-start_timestamp
        delta_t *= 1000
        print("Gesture Length: %f, Num:%d"%(delta_t,max_tap))
        if delta_t < tap_timings[max_tap]:
            return ('tap_click', max_tap)
            #This is a good gesture! Let us report an event


