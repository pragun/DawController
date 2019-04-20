from touchpadstate import TouchPadState

#Gesture states

#1. Waiting: The gesture is waiting for the right set of conditions to start
#   For example, a tap-to-click gesture would wait for the finger to be
#   lifted rather quickly after it was put down for it to activate
#   All gestures begin as waiting

#2. Active: The gesture is active right now.
#   This could be just one single syn report for tap-to-click type gestures
#   of live as long as the user is scrolling for two-finger scroll

#3. Expired: The gesture can not happen now
#   By now, it means untill all the fingers are lifted up from the touchpad
#   and it is in some sense "reset" 

#Gesture state machine
# Each gesture is recalculated for its state after a syn report is received

class GestureLifeStage(Enum):
    WAITING = 0
    ACTIVE = 1
    EXPIRED = 2

#Simple alias for such a long name
gls = GestureLifeStage
    
def Gesture(object):
    def __init__(self,tp_initial_state):
        self.life_state = gls.waiting
        self.tp_initial_state = tp_initial_state


