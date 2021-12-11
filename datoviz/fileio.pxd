from ._types cimport *

cdef extern from "<datoviz/fileio.h>":
    int dvz_write_png(const char* filename, uint32_t width, uint32_t height, const uint8_t* image)
    int dvz_make_png(uint32_t width, uint32_t height, const uint8_t* rgb, DvzSize* size, void** out)
