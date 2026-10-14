"""Send WM_DELETE_WINDOW only after matching the test child's X11 PID.

Opt-in diagnostic for smoke_studio_lifecycle; caller applies a3s timeout.
Never destroys a window, kills a client, or falls back to another window.
Original ctypes binding against the installed Xlib API, not copied Xlib code.
"""
import argparse
import ctypes as c
import ctypes.util
import json


class MessageData(c.Union):
    _fields_ = [("bytes", c.c_char * 20), ("shorts", c.c_short * 10), ("longs", c.c_long * 5)]


class ClientMessage(c.Structure):
    _fields_ = [("type", c.c_int), ("serial", c.c_ulong), ("send_event", c.c_int),
                ("display", c.c_void_p), ("window", c.c_ulong), ("message_type", c.c_ulong),
                ("format", c.c_int), ("data", MessageData)]


class Event(c.Union):
    _fields_ = [("client", ClientMessage), ("pad", c.c_long * 24)]


def target_id(identifier, pid):
    if not isinstance(identifier, str) or not identifier.startswith("0x"):
        raise ValueError("requires an explicit hexadecimal window ID")
    if not identifier[2:] or any(char not in "0123456789abcdefABCDEF" for char in identifier[2:]):
        raise ValueError("invalid window ID")
    window = int(identifier, 16)
    if not 0 < window <= 0xffffffff or not 0 < pid <= 0x7fffffff:
        raise ValueError("invalid window ID or PID")
    return window


def delete_event(display, window, protocols, delete):
    event = Event()
    event.client.type = 33  # ClientMessage, X11/X.h
    event.client.display = display
    event.client.window = window
    event.client.message_type = protocols
    event.client.format = 32
    event.client.data.longs[0] = delete
    event.client.data.longs[1] = 0  # CurrentTime
    return event


def send_owned_close(identifier, pid):
    window = target_id(identifier, pid)
    library = ctypes.util.find_library("X11")
    if not library:
        raise RuntimeError("libX11 is unavailable")
    x = c.CDLL(library)
    signatures = {
        "XOpenDisplay": ([c.c_char_p], c.c_void_p),
        "XCloseDisplay": ([c.c_void_p], c.c_int),
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
        def atom(name):
            value = x.XInternAtom(display, name.encode("ascii"), 1)
            if not value:
                raise RuntimeError("required X11 atom is absent: " + name)
            return value

        def property_values(name, expected_type):
            actual, count, remaining = c.c_ulong(), c.c_ulong(), c.c_ulong()
            format_bits = c.c_int()
            data = c.POINTER(c.c_ubyte)()
            status = x.XGetWindowProperty(display, window, atom(name), 0, 64, 0,
                                         expected_type, c.byref(actual), c.byref(format_bits),
                                         c.byref(count), c.byref(remaining), c.byref(data))
            try:
                if (status != 0 or actual.value != expected_type or format_bits.value != 32
                        or remaining.value != 0 or count.value > 64 or not data):
                    raise RuntimeError("invalid X11 ownership/protocol property: " + name)
                # Xlib represents each format32 item as a native unsigned long.
                values = c.cast(data, c.POINTER(c.c_ulong))
                return [values[i] for i in range(count.value)]
            finally:
                if data:
                    x.XFree(data)

        owner = property_values("_NET_WM_PID", atom("CARDINAL"))
        if owner != [pid]:
            raise RuntimeError("window does not belong to the requested test child")
        protocols, delete = atom("WM_PROTOCOLS"), atom("WM_DELETE_WINDOW")
        if delete not in property_values("WM_PROTOCOLS", atom("ATOM")):
            raise RuntimeError("window does not advertise WM_DELETE_WINDOW")
        event = delete_event(display, window, protocols, delete)
        if not x.XSendEvent(display, window, 0, 0, c.byref(event)):
            raise RuntimeError("XSendEvent could not encode the close request")
        x.XSync(display, 0)
        return dict(window=identifier, verifiedPid=pid, protocol="WM_DELETE_WINDOW", sent=True)
    finally:
        x.XCloseDisplay(display)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--window", required=True)
    parser.add_argument("--pid", type=int, required=True)
    args = parser.parse_args()
    try:
        print(json.dumps(send_owned_close(args.window, args.pid)))
        return 0
    except (ValueError, OSError, RuntimeError) as error:
        print(json.dumps(dict(error=str(error))))
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
