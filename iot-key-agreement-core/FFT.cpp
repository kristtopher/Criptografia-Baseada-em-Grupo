//
// Created by kristtopher on 19/11/2019.
//

#include "FFT.h"

FFT::FFT(void){}// Constructor

void FFT::Compute(float *vReal, float *vImag, uint16_t samples){
    Compute(vReal, vImag, samples, Exponent(samples));
}

void FFT::Compute(float *vReal, float *vImag, uint16_t samples, uint8_t power){
    uint16_t j = 0;
    for (uint16_t i = 0; i < (samples - 1); i++) {
        if (i < j) {
            Swap(&vReal[i], &vReal[j]);
        }
        uint16_t k = (samples >> 1);
        while (k <= j) {
            j -= k;
            k >>= 1;
        }
        j += k;
    }
    // Compute the FFT
    float c1 = -1.0;
    float c2 = 0.0;
    uint16_t l2 = 1;
    for (uint8_t l = 0; (l < power); l++) {
        uint16_t l1 = l2;
        l2 <<= 1;
        float u1 = 1.0;
        float u2 = 0.0;
        for (j = 0; j < l1; j++) {
            for (uint16_t i = j; i < samples; i += l2) {
                uint16_t i1 = i + l1;
                float t1 = u1 * vReal[i1] - u2 * vImag[i1];
                float t2 = u1 * vImag[i1] + u2 * vReal[i1];
                vReal[i1] = vReal[i] - t1;
                vImag[i1] = vImag[i] - t2;
                vReal[i] += t1;
                vImag[i] += t2;
            }
            float z = ((u1 * c1) - (u2 * c2));
            u2 = ((u1 * c2) + (u2 * c1));
            u1 = z;
        }
        c2 = sqrt((1.0 - c1) / 2.0);
        c2 = -c2;
        c1 = sqrt((1.0 + c1) / 2.0);
    }
}

void FFT::ComplexToMagnitude(float *vReal, float *vImag, uint16_t samples){
    // vM is half the size of vReal and vImag
    for (uint16_t i = 0; i < samples; i++)  vReal[i] = sqrt(sq(vReal[i]) + sq(vImag[i]));
}

void FFT::Windowing(float *vData, uint16_t samples){
    // Weighing factors are computed once before multiple use of FFT
    // The weighing function is symetric; half the weighs are recorded
    float samplesMinusOne = (float(samples) - 1.0);
    for (uint16_t i = 0; i < (samples >> 1); i++) {
        float indexMinusOne = float(i);
        float ratio = (indexMinusOne / samplesMinusOne);
        float weighingFactor = 1.0;
        // Compute and record weighting factor
        weighingFactor = 0.54 - (0.46 * cos(twoPi * ratio));
        vData[i] *= weighingFactor;
        vData[samples - (i + 1)] *= weighingFactor;
    }
}

uint8_t FFT::Exponent(uint16_t value){
    // Calculates the base 2 logarithm of a value
    uint8_t result = 0;
    while (((value >> result) & 1) != 1) result++;
    return(result);
}
// Private functions

void FFT::Swap(float *x, float *y){
    float temp = *x;
    *x = *y;
    *y = temp;
}