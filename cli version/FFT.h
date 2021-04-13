//
// Created by kristtopher on 19/11/2019.
//
#include <cmath>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#ifndef KEY_GENERATOR_FFT_FFT_H
#define KEY_GENERATOR_FFT_FFT_H

typedef unsigned char byte;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

/*Mathematial constants*/
#define twoPi 6.28318531
#define sq(x) ((x)*(x))
const uint16_t samples = 128; //This value MUST ALWAYS be a power of 2

class FFT {
public:
    /* Constructor */
    FFT(void);
    /* Functions */
    uint8_t Exponent(uint16_t value);
    void ComplexToMagnitude(float *vReal, float *vImag, uint16_t samples);
    void Compute(float *vReal, float *vImag, uint16_t samples);
    void Compute(float *vReal, float *vImag, uint16_t samples, uint8_t power);
    void Windowing(float *vData, uint16_t samples);
private:
    /* Variables */
    /* Functions */
    void Swap(float *x, float *y);
};
#endif //KEY_GENERATOR_FFT_FFT_H
