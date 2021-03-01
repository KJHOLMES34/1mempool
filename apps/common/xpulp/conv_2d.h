// Copyright 2020 ETH Zurich and University of Bologna.
//
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Author: Samuel Riedel, ETH Zurich
//         Davide Schiavone, ETH Zurich
//         Sergio Mazzola, ETH Zurich

#include "xpulp/builtins_v2.h"
#include <stdint.h>
#include <string.h>

#include "encoding.h"
#include "printf.h"
#include "runtime.h"
#include "synchronization.h"

// Define which kernel to use
//#define __XPULPIMG

/*
 * 2D Convolution 3x3 ----------------------------------
 * kernel     = conv2d_3x3_unrolled_i8_rv32im
 * data type  = 8-bit integer
 * multi-core = no
 * unrolling  = whole 3x3 kernel
 * simd       = no
 */
void conv2d_3x3_unrolled_i8_rv32im(int8_t const volatile *__restrict__ in, uint32_t in_x,
                                    uint32_t in_y, uint8_t const volatile *__restrict__ k,
                                    int32_t volatile *__restrict__ out) {
  int32_t sum;
  uint32_t weight = 0;
  for (int i = 0; i < 9; ++i) {
    weight += k[i];
  }

  for (uint32_t i = 1; i < in_x - 1; ++i) {
    for (uint32_t j = 1; j < in_y - 1; j++) {
      sum = 1;
      sum += in[(j - 1) * in_x + (i - 1)] * k[0];
      sum += in[(j - 1) * in_x + (i + 0)] * k[1];
      sum += in[(j - 1) * in_x + (i + 1)] * k[2];
      sum += in[(j + 0) * in_x + (i - 1)] * k[3];
      sum += in[(j + 0) * in_x + (i + 0)] * k[4];
      sum += in[(j + 0) * in_x + (i + 1)] * k[5];
      sum += in[(j + 1) * in_x + (i - 1)] * k[6];
      sum += in[(j + 1) * in_x + (i + 0)] * k[7];
      sum += in[(j + 1) * in_x + (i + 1)] * k[8];
      out[j * in_x + i] = sum / weight;
    }
  }
}

/*
 * 2D Convolution 3x3 ----------------------------------
 * kernel     = conv_3x3_unrolled_i8_xpulpv2
 * data type  = 8-bit integer
 * multi-core = no
 * unrolling  = whole 3x3 kernel
 * simd       = yes, Xpulpv2 intrinsics
 *
 * Insipired from Conv3x3_Vector from pulp-training
 */
void conv2d_3x3_unrolled_i8_xpulpv2(int8_t const volatile *__restrict__ In_Img, int32_t volatile *__restrict__ Out_Img,
                                    uint32_t R, uint32_t C, uint8_t const volatile *__restrict__ Kernel){
#ifdef __XPULPIMG
  v4s coeff_0, coeff_1, coeff_2;
  v4s Img_0, Img_1, Img_2;
  v4s new_data;
  uint32_t r, c, t;
  volatile int32_t S;
  static v4s and_mask = {0xFF, 0xFF, 0xFF, 0x00};

  uint32_t weight = 0;
  for (int i = 0; i < 9; ++i) {
    weight += Kernel[i];
  }

  coeff_0 = (v4s){Kernel[0], Kernel[1], Kernel[2], 0};
  coeff_1 = (v4s){Kernel[3], Kernel[4], Kernel[5], 0};
  coeff_2 = (v4s){Kernel[6], Kernel[7], Kernel[8], 0};

  // image board is black
  for (c = 1; c < C - 1; c++) {

    Img_0 = (v4s){In_Img[c - 1],         In_Img[c],         In_Img[c + 1],         0};
    Img_1 = (v4s){In_Img[c - 1 + R],     In_Img[c + R],     In_Img[c + 1 + R],     0};
    Img_2 = (v4s){In_Img[c - 1 + R * 2], In_Img[c + R * 2], In_Img[c + 1 + R * 2], 0};

    for (r = 1; r < R - 1; r++) {
      t = r * R + c;
      S = __builtin_pulp_dotsp4(Img_0, coeff_0);
      S = __builtin_pulp_sdotsp4(Img_1, coeff_1, S);
      S = __builtin_pulp_sdotsp4(Img_2, coeff_2, S);

      Out_Img[t] = S/weight;

      // load a new rod
      new_data = (v4s){In_Img[(r + 2) * R + c - 1], In_Img[(r + 2) * R + c], In_Img[(r + 2) * R + c + 1], 0};
      // move the window: move each vector one line down
      Img_0 = Img_1;
      Img_1 = Img_2;
      Img_2 = new_data;
    }
  }
#endif
}

// Testing
// Initialize the image in parallel
void init_conv2d_image_i8(int8_t *img, uint32_t img_x, uint32_t img_y) {
  if (img_y > img_x) {
    for (int i = 0; i < img_y; ++i) {
      for (int j = 0; j < img_x; ++j) {
        img[i * img_x + j] = (i % 16) + (j % 4);
      }
    }
  } else {
    for (int j = 0; j < img_x; ++j) {
      for (int i = 0; i < img_y; ++i) {
        img[i * img_x + j] = (i % 16) + (j % 4);
      }
    }
  }
}

// Verify and reset the image
int verify_conv2d_image_i8(int32_t *img, uint32_t img_x, uint32_t img_y) {
  for (int i = 1; i < img_y - 1; ++i) {
    int32_t y = i % 16;
    if (i % 16 == 0)
      y = 4;
    if (i % 16 == 15)
      y = 11;
    for (int32_t j = 1; j < img_x - 1; ++j) {
      int32_t x = ((j % 4) / 2) + 1;
      if ((int32_t)img[i * img_x + j] != (int32_t)(x + y)) {
        return (i + j) == 0 ? -1 : i * img_x + j;
      }
      img[i * img_x + j] = 0;
    }
  }
  return 0;
}

// Verify and reset the image
int verify_conv2d_image_i8_verbose(int32_t *img, uint32_t img_x, uint32_t img_y) {
  for (int i = 1; i < img_y - 1; ++i) {
    int32_t y = i % 16;
    if (i % 16 == 0)
      y = 4;
    if (i % 16 == 15)
      y = 11;
    printf("|");
    for (int32_t j = 1; j < img_x - 1; ++j) {
      int32_t x = ((j % 4) / 2) + 1;
      printf(" %2u - %2u |", img[i * img_x + j], x + y);
    }
    printf("\n");
  }
  return 0;
}

void conv2d_3x3_unrolled_i8_xpulpv2_verbose(int8_t const *__restrict__ In_Img, int32_t volatile *__restrict__ Out_Img,
                                    uint32_t R, uint32_t C, uint8_t const volatile *__restrict__ Kernel){
#ifdef __XPULPIMG
  v4s coeff_0, coeff_1, coeff_2;
  v4s Img_0, Img_1, Img_2;
  v4s new_data;
  uint32_t r, c, t;
  volatile int32_t S;

  uint32_t weight = 0;
  for (int i = 0; i < 9; ++i) {
    weight += Kernel[i];
  }

  // __asm__ volatile(
  //   "lw %[c0], 0(%[addr_ker]) \n\t"
  //   "lw %[c1], 3(%[addr_ker]) \n\t"
  //   "lw %[c2], 6(%[addr_ker]) \n\t"
  //   : [ c0 ] "=&r"(coeff_0), [ c1 ] "=&r"(coeff_1), [ c2 ] "=&r"(coeff_2)
  //   : [ addr_ker ] "r"(Kernel)
  //   : "memory");
  //
  // coeff_0 = coeff_0 & 0xFFFFFF00;
  // coeff_1 = coeff_1 & 0xFFFFFF00;
  // coeff_2 = coeff_2 & 0xFFFFFF00;

  coeff_0 = (v4s){Kernel[0], Kernel[1], Kernel[2], 0};
  coeff_1 = (v4s){Kernel[3], Kernel[4], Kernel[5], 0};
  coeff_2 = (v4s){Kernel[6], Kernel[7], Kernel[8], 0};

  // image board is black
  for (c = 1; c < C - 1; c++) {

    Img_0 = (v4s){In_Img[c - 1],         In_Img[c],         In_Img[c + 1],         0};
    Img_1 = (v4s){In_Img[c - 1 + R],     In_Img[c + R],     In_Img[c + 1 + R],     0};
    Img_2 = (v4s){In_Img[c - 1 + R * 2], In_Img[c + R * 2], In_Img[c + 1 + R * 2], 0};

    for (r = 1; r < R - 1; r++) {
      printf("-------------\n");

      printf("[ %u, %u, %u]\n", Img_0[0], Img_0[1], Img_0[2]);
      printf("[ %u, %u, %u]\n", Img_1[0], Img_1[1], Img_1[2]);
      printf("[ %u, %u, %u]\n", Img_2[0], Img_2[1], Img_2[2]);

      t = r * R + c;
      S = __builtin_pulp_dotsp4(Img_0, coeff_0);
      S = __builtin_pulp_sdotsp4(Img_1, coeff_1, S);
      S = __builtin_pulp_sdotsp4(Img_2, coeff_2, S);

      printf("S = %d\n", S);
      printf("S/weight = %d\n", S/weight);

      Out_Img[t] = S/weight;
      printf("Out_Img[%d] = %d\n", t, Out_Img[t]);

      new_data = (v4s){In_Img[(r + 2) * R + c - 1], In_Img[(r + 2) * R + c], In_Img[(r + 2) * R + c + 1], 0};

      // Move the window
      /*
        Three vectors:
        Img_0 = {A0, A1, A2, 0}
        Img_1 = {B0, B1, B2, 0}
        Img_2 = {C0, C1, C2, 0}
        Current Windonw:
        XX XX XX
        A0 A1 A2
        B0 B1 B2
        C0 C1 C2
        D0 D1 D2
        XX XX XX
        We want to load next line (D0, D1, D2) in vector new_data
        new_data = {D0, D1, D2, 0}
        Move each vector one line down
        Img_0 = Img_1
        Img_1 = Img_2
        Img_2 = new_data
      */

      Img_0 = Img_1;
      Img_1 = Img_2;
      Img_2 = new_data;
    }
  }
#endif
}
