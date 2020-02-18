#ifndef DETECTEUR8PAR8
#define DETECTEUR8PAR8

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <immintrin.h>

using namespace std;

class Detecteur8par8{

private :

public :
	Detecteur8par8();
	~Detecteur8par8();
    void detection(float* ps, float* buffer); 

};

Detecteur8par8::Detecteur8par8(){}

Detecteur8par8::~Detecteur8par8(){}

void Detecteur8par8::detection(float* ps, float *buffer){

	/* AVX
	add
	add
    add
    add
    ... 7 add total
	*/

	__m256 v_ps2 = _mm256_loadu_ps(buffer); //[0]
    __m256 v_ps = _mm256_loadu_ps(buffer + 1); //[1]
    v_ps2 = _mm256_add_ps(v_ps2, v_ps);
    v_ps = _mm256_loadu_ps(buffer + 3); //[3]
    v_ps2 = _mm256_add_ps(v_ps2, v_ps);
    v_ps = _mm256_loadu_ps(buffer + 4); //[4]
    v_ps2 = _mm256_add_ps(v_ps2, v_ps);
    v_ps = _mm256_loadu_ps(buffer + 14); //[14]
    v_ps2 = _mm256_add_ps(v_ps2, v_ps);
    v_ps = _mm256_loadu_ps(buffer + 15); //[15]
    v_ps2 = _mm256_add_ps(v_ps2, v_ps);
    v_ps = _mm256_loadu_ps(buffer + 18); //[18]
    v_ps2 = _mm256_add_ps(v_ps2, v_ps);
    v_ps = _mm256_loadu_ps(buffer + 19); //[19]
    v_ps2 = _mm256_add_ps(v_ps2, v_ps);
	/* AVX
	carre des elmts
	add
    carre des elmts
	add
    ... 32 fois
	mul 8
    sqrt

    div
	*/
    
	__m256 v_buf1 = _mm256_loadu_ps(buffer); //[0]
	v_buf1 =  _mm256_mul_ps(v_buf1, v_buf1); 
    for (int k=1; k<32; k++) {
        __m256 v_buf2 = _mm256_loadu_ps((buffer + k)); //[k]
        v_buf2 =  _mm256_mul_ps(v_buf2, v_buf2);
        v_buf1 = _mm256_add_ps(v_buf1, v_buf2);
    }
    __attribute__ ((aligned (16))) float huit[8] = {8, 8, 8, 8, 8, 8, 8, 8};
    __m256 v_huit = _mm256_load_ps(huit);
    //__m256i v_buf3 = _mm256_cvtps_epi32 (v_buf1);
    //v_buf3 = _mm256_slli_epi32(v_buf3, 2); //x8
    //v_buf1 = _mm256_cvtepi32_ps (v_buf3);
    v_buf1 = _mm256_mul_ps(v_buf1, v_huit); //x8
    v_buf1 = _mm256_sqrt_ps(v_buf1); //sqrt

    v_buf1 = _mm256_div_ps(v_ps2, v_buf1); //div
	_mm256_storeu_ps(ps, v_buf1);
}

#endif