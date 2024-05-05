
# https://github.com/thiagofe/ulab_samples
# has bechmarks for FFT
# 1024 samples, 2.0 ms

# https://github.com/fakufaku/esp32-fft/blob/master/performance/performance.csv
# 1024	0.971210 ms

# https://espressif-docs.readthedocs-hosted.com/projects/esp-dsp/en/latest/esp-dsp-benchmarks.html
# 75547 to 172664 samples
# 0.31 to 0.73 ms @ 240 Mhz

# https://mecha-mind.medium.com/fast-fourier-transform-optimizations-5c1fd108a8ed

import math
import cmath
import time
import array
import gc

# Import the different implementations
from fft_python import FFTPreInplace

ulab = None
try:
    import ulab
    from ulab import numpy
    pass
except ImportError as e:
    print(e)

emlfft = None
try:
    import emlfft
except ImportError as e:
    print(e)


def make_two_sines(f1 = 2.0, f2 = 20.0, sr = 100, dur = 1.0):
    np = numpy

    t = np.linspace(0, 1, num=int(dur*sr))
    sig = np.sin(2*np.pi*f1*t) + np.sin(2*np.pi*f2*t)

    return t, sig


def run_one(data, imag, n, repeat=10):

    assert len(data) == n

    ulab = False
    emlearn = True
    pyfft = False
    
    re = array.array('f', data)
    im = array.array('f', imag)

    # Python
    fft = FFTPreInplace(n)

    if pyfft:
        start = time.ticks_us()
        for i in range(repeat):
            fft.compute(re, im)
            #out = fft_optimized(data, seq)
        d = ((time.ticks_diff(time.ticks_us(), start)) / repeat) / 1000.0 # ms
        print('python', d)

    # ulab
    if ulab:    
        start = time.ticks_us()
        for i in range(repeat):
            out, _ = numpy.fft.fft(data)
        d = ((time.ticks_diff(time.ticks_us(), start)) / repeat) / 1000.0 # ms
        print('ulab', d)

    # emlearn
    if emlearn:
        fft = emlfft.new(n)
        emlfft.fill(fft, n)

        start = time.ticks_us()
        for n in range(repeat):
            out = fft.run(re, im)
        d = ((time.ticks_diff(time.ticks_us(), start)) / repeat) / 1000.0 # ms
        print('emlearn', d)

def run_all():

    lengths = [
        32,
        #128,
        #256,
        #512,
        #1024,
    ]

    sines = make_two_sines(dur=20.0)

    for n in lengths:
        data = sines[0][0:n]
        imag = numpy.zeros(data.shape, dtype=data.dtype)
        run_one(data, imag, n)
        gc.collect()


if __name__ == '__main__':
    run_all()
