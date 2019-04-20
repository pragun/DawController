def fizz(cls):
    a = [(attr,val) for attr,val in cls.__dict__.items() if hasattr(val,'g')]
    for attr,val in a:
        if callable(val) and not attr.startswith("__"):
            if hasattr(val,'g'):
                setattr(cls,attr+'_mizzed',mizzed(val))
                print(val)
    return cls


def mizzed(func):
    def mizzzah(*args,**kwargs):
        print("mizzedd..")
        func(*args,**kwargs)
    return staticmethod(mizzzah)


def mizz(func):
    class PP(object):
        def __init__(self):
            self.g = 'g'
        def __call__(*args,**kwargs):
            func(*args,**kwargs)
    return PP()


@fizz
class A(object):
    def __init__(self):
        self.decorated_functions = []

    @mizz
    def aa(self):
        print("aa")

    @mizz
    def bb(self):
        print("bb")
        

A.aa_mizzed()
A.bb_mizzed()
