"""
This file is automatically included at the end of _ctypes.py during the ctypes wrapper generation.
This is handled by `just ctypes`.
"""

# ===============================================================================
# Error handling
# ===============================================================================


class DatovizError(Exception):
    pass


@DvzErrorCallback
def error_handler(message):
    raise DatovizError(message.decode('utf-8'))


error_callback(error_handler)
