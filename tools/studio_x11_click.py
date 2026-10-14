"""Original Xlib binding: click only the header of an explicitly owned test window.

Does not move the user's pointer, change focus or send events to other clients.
Uses the same PID/window guard as studio_x11_close, but no close/kill fallback.
"""
import ctypes as c
import ctypes.util
from studio_x11_close import target_id


class ButtonEvent(c.Structure):
    _fields_ = [("type", c.c_int), ("serial", c.c_ulong), ("send_event", c.c_int),
                ("display", c.c_void_p), ("window", c.c_ulong), ("root", c.c_ulong),
                ("subwindow", c.c_ulong), ("time", c.c_ulong),
                ("x", c.c_int), ("y", c.c_int), ("x_root", c.c_int), ("y_root", c.c_int),
                ("state", c.c_uint), ("button", c.c_uint), ("same_screen", c.c_int)]


class Event(c.Union):
    _fields_ = [("button", ButtonEvent), ("pad", c.c_long * 24)]


def click_header(identifier, pid, x_position, y_position):
    window = target_id(identifier, pid)
    if not (0 <= x_position < 10000 and 0 <= y_position < 80):
        raise ValueError("header coordinates outside the allowed region")
    x = c.CDLL(ctypes.util.find_library("X11") or "libX11.so.6")
    signatures = {
        "XOpenDisplay": ([c.c_char_p], c.c_void_p),
        "XCloseDisplay": ([c.c_void_p], c.c_int),
        "XDefaultRootWindow": ([c.c_void_p], c.c_ulong),
        "XInternAtom": ([c.c_void_p, c.c_char_p, c.c_int], c.c_ulong),
        "XGetWindowProperty": ([c.c_void_p, c.c_ulong, c.c_ulong, c.c_long, c.c_long,
                                 c.c_int, c.c_ulong, c.POINTER(c.c_ulong), c.POINTER(c.c_int),
                                 c.POINTER(c.c_ulong), c.POINTER(c.c_ulong),
                                 c.POINTER(c.POINTER(c.c_ubyte))], c.c_int),
        "XFree": ([c.c_void_p], c.c_int),
        "XSendEvent": ([c.c_void_p, c.c_ulong, c.c_int, c.c_long, c.POINTER(Event)], c.c_int),
        "XSync": ([c.c_void_p, c.c_int], c.c_int),
    }
    for name, (arguments, result) in signatures.items():
        function = getattr(x, name)
        function.argtypes, function.restype = arguments, result
    display = x.XOpenDisplay(None)
    if not display:
        raise RuntimeError("cannot open the X11 display")
    try:
        pid_atom = x.XInternAtom(display, b"_NET_WM_PID", 1)
        cardinal = x.XInternAtom(display, b"CARDINAL", 1)
        if not pid_atom or not cardinal:
            raise RuntimeError("ownership atoms unavailable")
        actual, count, remaining, bits = c.c_ulong(), c.c_ulong(), c.c_ulong(), c.c_int()
        data = c.POINTER(c.c_ubyte)()
        status = x.XGetWindowProperty(display, window, pid_atom, 0, 1, 0, cardinal,
                                     c.byref(actual), c.byref(bits), c.byref(count),
                                     c.byref(remaining), c.byref(data))
        try:
            if (status or actual.value != cardinal or bits.value != 32 or count.value != 1
                    or remaining.value or not data or c.cast(data, c.POINTER(c.c_ulong))[0] != pid):
                raise RuntimeError("window does not belong to the requested test child")
        finally:
            if data:
                x.XFree(data)
        for event_type, state, mask in ((4, 0, 1 << 2), (5, 1 << 8, 1 << 3)):
            event = Event()
            event.button.type, event.button.send_event = event_type, 1
            event.button.display, event.button.window = display, window
            event.button.root = x.XDefaultRootWindow(display)
            event.button.x, event.button.y = x_position, y_position
            event.button.button, event.button.state, event.button.same_screen = 1, state, 1
            if not x.XSendEvent(display, window, 0, mask, c.byref(event)):
                raise RuntimeError("header click event failed")
        x.XSync(display, 0)
    finally:
        x.XCloseDisplay(display)

