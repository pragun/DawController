import math

class FingerPosition(object):
    def __init__(self,slot):
        self.touch = 0 #True if the finger has not yet been lifted up
        self.slot = slot
        self.abs_x = 0
        self.abs_y = 0
        self.touch_major = 0
        self.touch_minor = 0
        self.width_major = 0
        self.width_minor = 0
        self.pressure = 0
        self.orientation = 0
        self.tracking_id = 0

    @classmethod
    def copy(cls,i):
        s = cls(i.slot)
        s.touch       = i.touch       
        s.slot        = i.slot       
        s.abs_x       = i.abs_x      
        s.abs_y       = i.abs_y      
        s.touch_major = i.touch_major
        s.touch_minor = i.touch_minor
        s.width_major = i.width_major
        s.width_minor = i.width_minor
        s.pressure    = i.pressure   
        s.orientation = i.orientation
        s.tracking_id = i.tracking_id
        return s
    
    def distance(self,i):
        #Make sure that it is the same slot
        if self.slot != i.slot:
            raise ('Tried Distance between FingerPositions on different slots')
        else:
            delx = self.abs_x - i.abs_x
            dely = self.abs_y - i.abs_y
            return math.sqrt((delx**2)+(dely**2))
    
    def repr_all_touch_data(self):
        return '|S:{:1d}| T:{:1d}| X:{:6d}| Y:{:6d}| TMj:{:6d}| TMn:{:6d}| WMj:{:6d}| Wmn:{:6d}| P:{:6d}| O:{:6d}| Tid:{:6d}'.format(self.slot,self.touch,self.abs_x,self.abs_y,self.touch_major,self.touch_minor,self.width_major,self.width_minor,self.pressure,self.orientation, self.tracking_id)

    def __repr__(self):
        return self.repr_all_touch_data()

    
