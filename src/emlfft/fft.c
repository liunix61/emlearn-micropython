// Include the header file to get access to the MicroPython API
#include "py/dynruntime.h"

#include <eml_fft.h>

#include <string.h>

// memset is used by some standard C constructs
#if !defined(__linux__)
void *memcpy(void *dst, const void *src, size_t n) {
    return mp_fun_table.memmove_(dst, src, n);
}
void *memset(void *s, int c, size_t n) {
    return mp_fun_table.memset_(s, c, n);
}

void NORETURN abort() {
    while (1) {
        ;
    }
}
#endif


// MicroPython type for EmlFFT
typedef struct _mp_obj_fft_t {
    mp_obj_base_t base;
    EmlFFT fft;
} mp_obj_fft_t;

mp_obj_full_type_t fft_type;

// Create a new instance
static mp_obj_t fft_new(mp_obj_t length_obj) {

    const int fft_length = mp_obj_get_int(length_obj);

    // Construct object
    mp_obj_fft_t *o = mp_obj_malloc(mp_obj_fft_t, (mp_obj_type_t *)&fft_type);
    EmlFFT self = o->fft;
    //memset(self, 0, sizeof(EmlIIR)); // HACK: try to get memset symbol in

    const int table_length = fft_length / 2;
    self.cos = (float *)m_malloc(sizeof(float)*table_length);
    self.sin = (float *)m_malloc(sizeof(float)*table_length);

    const EmlError err = eml_fft_fill(self, fft_length);
    if (err != EmlOk) {
        m_free(self.cos);
        m_free(self.sin);
        mp_raise_ValueError(MP_ERROR_TEXT("EmlError"));
    }

    return MP_OBJ_FROM_PTR(o);
}
static MP_DEFINE_CONST_FUN_OBJ_1(fft_new_obj, fft_new);

// Delete the instance
static mp_obj_t fft_del(mp_obj_t self_obj) {

    mp_obj_fft_t *o = MP_OBJ_TO_PTR(self_obj);
    EmlFFT self = o->fft;

    // free allocated data
    m_free(self.cos);
    m_free(self.sin);

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(fft_del_obj, fft_del);


float *
check_extract_array(mp_obj_t obj, int length) {

    // Verify real
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(obj, &bufinfo, MP_BUFFER_RW);
    if (bufinfo.typecode != 'f') {
        mp_raise_ValueError(MP_ERROR_TEXT("expecting float array"));
        return NULL;
    }
    float *values = bufinfo.buf;
    const int array_length = bufinfo.len / sizeof(*values);

    if (array_length != length) {
        mp_raise_ValueError(MP_ERROR_TEXT("wrong array length"));
        return NULL;
    }

    return values;
}

// Add a node to the tree
static mp_obj_t fft_run(mp_obj_t self_obj, mp_obj_t real_obj, mp_obj_t imag_obj) {

    mp_obj_fft_t *o = MP_OBJ_TO_PTR(self_obj);
    EmlFFT self = o->fft;
    const int fft_length = self.length*2;

    float *real_values = check_extract_array(real_obj, fft_length);
    float *imag_values = check_extract_array(imag_obj, fft_length);

    const EmlError err = eml_fft_forward(self, real_values, imag_values, fft_length);
    if (err != EmlOk) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("eml_fft_forward error"));
    }

    return mp_const_none;
 }
static MP_DEFINE_CONST_FUN_OBJ_3(fft_run_obj, fft_run);


mp_map_elem_t fft_locals_dict_table[2];
static MP_DEFINE_CONST_DICT(fft_locals_dict, fft_locals_dict_table);

// This is the entry point and is called when the module is imported
mp_obj_t mpy_init(mp_obj_fun_bc_t *self, size_t n_args, size_t n_kw, mp_obj_t *args) {
    // This must be first, it sets up the globals dict and other things
    MP_DYNRUNTIME_INIT_ENTRY

    mp_store_global(MP_QSTR_new, MP_OBJ_FROM_PTR(&fft_new_obj));

    fft_type.base.type = (void*)&mp_fun_table.type_type;
    fft_type.flags = MP_TYPE_FLAG_ITER_IS_CUSTOM;
    fft_type.name = MP_QSTR_emlfft;
    // methods
    fft_locals_dict_table[0] = (mp_map_elem_t){ MP_OBJ_NEW_QSTR(MP_QSTR_run), MP_OBJ_FROM_PTR(&fft_run_obj) };
    fft_locals_dict_table[1] = (mp_map_elem_t){ MP_OBJ_NEW_QSTR(MP_QSTR___del__), MP_OBJ_FROM_PTR(&fft_del_obj) };

    MP_OBJ_TYPE_SET_SLOT(&fft_type, locals_dict, (void*)&fft_locals_dict, 2);

    // This must be last, it restores the globals dict
    MP_DYNRUNTIME_INIT_EXIT
}

