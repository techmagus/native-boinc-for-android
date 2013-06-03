/******************
 *
 * analyzeFuncs_SSE3.cpp
 * 
 * Description: Intel SSE3 (Prescott New Instruction) optimzized functions
 * CPUs: Intel Pentium IV Model 3+
 *
 */

// Copyright 2007 Regents of the University of California

// SETI_BOINC is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2, or (at your option) any later
// version.

// SETI_BOINC is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.

// You should have received a copy of the GNU General Public License along
// with SETI_BOINC; see the file COPYING.  If not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#include "sah_config.h"

// The following is empty if USE_INTRINSICS is not defined
#ifdef USE_INTRINSICS

#include <climits>
#include <cmath>

#ifdef HAVE_INTRIN_H
#include <intrin.h>
#endif
#ifdef HAVE_PMMINTRIN_H
#include <pmmintrin.h>
#endif
#include <xmmintrin.h>
#include <emmintrin.h>

#include "s_util.h"
#include "x86_float4.h"
#include "analyzeFuncs_vector.h"
#include "analyzeFuncs.h"


__inline __m128 vec_recip3(__m128 v) 
  {
  // obtain estimate
  __m128 estimate = _mm_rcp_ps( v );
  // one round of Newton-Raphson
  return _mm_add_ps(_mm_mul_ps(_mm_sub_ps(ONE, _mm_mul_ps(estimate, v)), estimate), estimate);
  }  

// =============================================================================
//
// sse3_vChirpData
// version by: Alex Kan
//   http://tbp.berkeley.edu/~alexkan/seti/
//
int sse3_ChirpData_ak(
  sah_complex * cx_DataArray,
  sah_complex * cx_ChirpDataArray,
  int chirp_rate_ind,
  double chirp_rate,
  int  ul_NumDataPoints,
  double sample_rate
) {
  int i;

  if (chirp_rate_ind == 0) {
    memcpy(cx_ChirpDataArray, cx_DataArray,  (int)ul_NumDataPoints * sizeof(sah_complex)  );
    return 0;
  }

  int vEnd;  
  double srate = chirp_rate * 0.5 / (sample_rate * sample_rate);
  __m128d rate = _mm_set1_pd(chirp_rate * 0.5 / (sample_rate * sample_rate));
  __m128d roundVal = _mm_set1_pd(srate >= 0.0 ? TWO_TO_52 : -TWO_TO_52);

  // main vectorised loop
  vEnd = ul_NumDataPoints - (ul_NumDataPoints & 3);
  for (i = 0; i < vEnd; i += 4) {
    const float *data = (const float *) (cx_DataArray + i);
    float *chirped = (float *) (cx_ChirpDataArray + i);
    __m128d di = _mm_set1_pd(i);
    __m128d a1 = _mm_add_pd(_mm_set_pd(1.0, 0.0), di);
    __m128d a2 = _mm_add_pd(_mm_set_pd(3.0, 2.0), di);

    __m128 d1, d2;
    __m128 cd1, cd2;
    __m128 td1, td2;
    __m128 x;
    __m128 y;
    __m128 s;
    __m128 c;
    __m128 m;

    // load the signal to be chirped
    prefetchnta((const void *)( data+32 ));
    d1 = _mm_load_ps(data);
    d2 = _mm_load_ps(data+4);

    // calculate the input angle
    a1 = _mm_mul_pd(_mm_mul_pd(a1, a1), rate);
    a2 = _mm_mul_pd(_mm_mul_pd(a2, a2), rate);

    // reduce the angle to the range (-0.5, 0.5)
    a1 = _mm_sub_pd(a1, _mm_sub_pd(_mm_add_pd(a1, roundVal), roundVal));
    a2 = _mm_sub_pd(a2, _mm_sub_pd(_mm_add_pd(a2, roundVal), roundVal));

    // convert pair of packed double into packed single
    x = _mm_movelh_ps(_mm_cvtpd_ps(a1), _mm_cvtpd_ps(a2));

    // square to the range [0, 0.25)
    y = _mm_mul_ps(x, x);

    // perform the initial polynomial approximations
    s = _mm_mul_ps(_mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(y, SS4),
                                    SS3),
                                y),
                          SS2),
                    y),
              SS1),
          x);
    c = _mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(y, CC3),
                                CC2),
                          y),
                    CC1),
              y),
          ONE);

    // perform first angle doubling
    x = _mm_sub_ps(_mm_mul_ps(c, c), _mm_mul_ps(s, s));
    y = _mm_mul_ps(_mm_mul_ps(s, c), TWO);

    // calculate scaling factor to correct the magnitude
    //      m1 = vec_nmsub(y1, y1, vec_nmsub(x1, x1, TWO));
    //      m2 = vec_nmsub(y2, y2, vec_nmsub(x2, x2, TWO));
    m = vec_recip3(_mm_add_ps(_mm_mul_ps(x, x), _mm_mul_ps(y, y)));

    // perform second angle doubling
    c = _mm_sub_ps(_mm_mul_ps(x, x), _mm_mul_ps(y, y));
    s = _mm_mul_ps(_mm_mul_ps(y, x), TWO);

    // correct the magnitude (final sine / cosine approximations)
    s = _mm_mul_ps(s, m);
    c = _mm_mul_ps(c, m);

    // chirp the data
    cd1 = _mm_shuffle_ps(c, c, 0x50);
    cd2 = _mm_shuffle_ps(c, c, 0xfa);
    cd1 = _mm_mul_ps(cd1, d1);
    cd2 = _mm_mul_ps(cd2, d2);
    d1 = _mm_shuffle_ps(d1, d1, 0xb1);
    d2 = _mm_shuffle_ps(d2, d2, 0xb1);
    td1 = _mm_shuffle_ps(s, s, 0x50);
    td2 = _mm_shuffle_ps(s, s, 0xfa);
    td1 = _mm_mul_ps(td1, d1);
    td2 = _mm_mul_ps(td2, d2);
    cd1 = _mm_addsub_ps(cd1, td1);
    cd2 = _mm_addsub_ps(cd2, td2);

    // store chirped values
    _mm_stream_ps(chirped, cd1);
    _mm_stream_ps(chirped+4, cd2);
  }
  _mm_sfence();

  // handle tail elements with scalar code
  for (   ; i < ul_NumDataPoints; ++i) {
    double angle = srate * i * i * 0.5;
    double s = sin(angle);
    double c = cos(angle);
    float re = cx_DataArray[i][0];
    float im = cx_DataArray[i][1];

    cx_ChirpDataArray[i][0] = re * c - im * s;
    cx_ChirpDataArray[i][1] = re * s + im * c;
  }
  analysis_state.FLOP_counter+=12.0*ul_NumDataPoints;

  return 0;
}

// =============================================================================
//
// JWS: alternate SSE3 chirp
//
// Using Mendenhall Faster SinCos, coding based on
// version 8 v_vChirpData for SSE3 by Alex Kan
//

int sse3_ChirpData_ak8(
    sah_complex * cx_DataArray,
    sah_complex * cx_ChirpDataArray,
    int chirp_rate_ind,
    double chirp_rate,
    int  ul_NumDataPoints,
    double sample_rate
) {
    int i;

    if (chirp_rate_ind == 0) {
      memcpy(cx_ChirpDataArray, cx_DataArray,  (int)ul_NumDataPoints * sizeof(sah_complex)  );
      return 0;
    }

    int vEnd;
    double srate = chirp_rate * 0.5 / (sample_rate * sample_rate);
    __m128d rate = _mm_set1_pd(chirp_rate * 0.5 / (sample_rate * sample_rate));
    __m128d roundVal = _mm_set1_pd(srate >= 0.0 ? TWO_TO_52 : -TWO_TO_52);
    __m128d DFOUR = _mm_set_pd(4.0, 4.0);


    // main vectorised loop
    vEnd = ul_NumDataPoints - (ul_NumDataPoints & 3);
    __m128d di1 = _mm_set_pd(2.0, 0.0);                 // set time patterns for eventual moveldup/movehdup
    __m128d di2 = _mm_set_pd(3.0, 1.0);

    for (i = 0; i < vEnd; i += 4) {
      const float *d = (const float *) (cx_DataArray + i);
      float *cd = (float *) (cx_ChirpDataArray + i);

      __m128d a1, a2;

      __m128 d1, d2;
      __m128 cd1, cd2;
      __m128 td1, td2;

      __m128 x;
      __m128 y;
      __m128 z;
      __m128 s;
      __m128 c;
      __m128 m;

      // load the signal to be chirped
      d1 = _mm_load_ps(d);
      d2 = _mm_load_ps(d+4);

      // calculate the input angle
      a1 = _mm_mul_pd(_mm_mul_pd(di1, di1), rate);
      a2 = _mm_mul_pd(_mm_mul_pd(di2, di2), rate);

      // update times for next
      di1 = _mm_add_pd(di1, DFOUR);
      di2 = _mm_add_pd(di2, DFOUR);

      // reduce the angle to the range (-0.5, 0.5)
      a1 = _mm_sub_pd(a1, _mm_sub_pd(_mm_add_pd(a1, roundVal), roundVal));
      a2 = _mm_sub_pd(a2, _mm_sub_pd(_mm_add_pd(a2, roundVal), roundVal));

      // convert pair of packed double into packed single
      x = _mm_movelh_ps(_mm_cvtpd_ps(a1), _mm_cvtpd_ps(a2));               // 3   1   2   0

      // square to the range [0, 0.25)
      y = _mm_mul_ps(x, x);

      // perform the initial polynomial approximations, Estrin's method
      z = _mm_mul_ps(y, y);

      s = _mm_mul_ps(_mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(y, SS4F),
                                                      SS3F),
                                           z),
                                _mm_add_ps(_mm_mul_ps(y, SS2F),
                                           SS1F)),
                     x);
      c = _mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(y, CC3F),
                                           CC2F),
                                z),
                     _mm_add_ps(_mm_mul_ps(y, CC1F),
                                ONE));

      // perform first angle doubling
      x = _mm_sub_ps(_mm_mul_ps(c, c), _mm_mul_ps(s, s));
      y = _mm_mul_ps(_mm_mul_ps(s, c), TWO);

      // calculate scaling factor to correct the magnitude
      m = _mm_sub_ps(_mm_sub_ps(TWO, _mm_mul_ps(x, x)), _mm_mul_ps(y, y));

      // perform second angle doubling
      c = _mm_sub_ps(_mm_mul_ps(x, x), _mm_mul_ps(y, y));
      s = _mm_mul_ps(_mm_mul_ps(y, x), TWO);

      // correct the magnitude (final sine / cosine approximations)
      c = _mm_mul_ps(c, m);                                       // c3    c1    c2    c0
      s = _mm_mul_ps(s, m);

      // chirp the data
      cd1 = _mm_moveldup_ps(c);                                   // c1    c1    c0    c0
      cd2 = _mm_movehdup_ps(c);                                   // c3    c3    c2    c2
      cd1 = _mm_mul_ps(cd1, d1);                                  // c1.i1 c1.r1 c0.i0 c0.r0
      cd2 = _mm_mul_ps(cd2, d2);                                  // c3.i3 c3.r3 c2.i2 c2.r2
      d1 = _mm_shuffle_ps(d1, d1, 0xb1);
      d2 = _mm_shuffle_ps(d2, d2, 0xb1);
      td1 = _mm_moveldup_ps(s);
      td2 = _mm_movehdup_ps(s);
      td1 = _mm_mul_ps(td1, d1);
      td2 = _mm_mul_ps(td2, d2);
      cd1 = _mm_addsub_ps(cd1, td1);
      cd2 = _mm_addsub_ps(cd2, td2);

      // store chirped values
      _mm_stream_ps(cd, cd1);
      _mm_stream_ps(cd+4, cd2);
    }

    // handle tail elements with scalar code
    for (; i < ul_NumDataPoints; ++i) {
      double angle = srate * i * i * 0.5;
      double s = sin(angle);
      double c = cos(angle);

      float re = cx_DataArray[i][0];
      float im = cx_DataArray[i][1];

      cx_ChirpDataArray[i][0] = re * c - im * s;
      cx_ChirpDataArray[i][1] = re * s + im * c;
    }
    analysis_state.FLOP_counter+=12.0*ul_NumDataPoints;
    return 0;
}



#endif // USE_INTRINSICS
