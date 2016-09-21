/******************************************************************************
 *
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************
 * Originally developed and contributed by Imagination Technologies
*/

#ifndef __IMPEG2_MACROS_MSA_H__
#define __IMPEG2_MACROS_MSA_H__

#include <stdint.h>
#include "iv_datatypedef.h"

#if defined(__clang__)
    #define CLANG_BUILD
#endif

#ifdef CLANG_BUILD
    typedef signed char v16i8 __attribute__((vector_size(16), aligned(16)));
    typedef unsigned char v16u8 __attribute__((vector_size(16), aligned(16)));
    typedef short v8i16 __attribute__((vector_size(16), aligned(16)));
    typedef unsigned short v8u16 __attribute__((vector_size(16), aligned(16)));
    typedef int v4i32 __attribute__((vector_size(16), aligned(16)));
    typedef unsigned int v4u32 __attribute__((vector_size(16), aligned(16)));
    typedef long long v2i64 __attribute__((vector_size(16), aligned(16)));
    typedef unsigned long long v2u64 __attribute__((vector_size(16), aligned(16)));

    #define __msa_srari_h   __builtin_msa_srari_h
    #define __msa_srari_w   __builtin_msa_srari_w
    #define __msa_maxi_s_h  __builtin_msa_maxi_s_h
    #define __msa_min_s_h   __builtin_msa_min_s_h
    #define __msa_clti_s_h  __builtin_msa_clti_s_h
    #define __msa_aver_u_b  __builtin_msa_aver_u_b
    #define __msa_hadd_u_h  __builtin_msa_hadd_u_h
    #define __msa_dotp_s_w  __builtin_msa_dotp_s_w
    #define __msa_dpadd_s_w __builtin_msa_dpadd_s_w
    #define __msa_sldi_b    __builtin_msa_sldi_b
    #define __msa_pckev_b   __builtin_msa_pckev_b
    #define __msa_pckev_h   __builtin_msa_pckev_h
    #define __msa_pckev_d   __builtin_msa_pckev_d
    #define __msa_pckod_d   __builtin_msa_pckod_d
    #define __msa_ilvl_b    __builtin_msa_ilvl_b
    #define __msa_ilvl_h    __builtin_msa_ilvl_h
    #define __msa_ilvl_w    __builtin_msa_ilvl_w
    #define __msa_ilvl_d    __builtin_msa_ilvl_d
    #define __msa_ilvr_b    __builtin_msa_ilvr_b
    #define __msa_ilvr_h    __builtin_msa_ilvr_h
    #define __msa_ilvr_w    __builtin_msa_ilvr_w
    #define __msa_ilvr_d    __builtin_msa_ilvr_d
    #define __msa_ilvev_h   __builtin_msa_ilvev_h
    #define __msa_fill_b    __builtin_msa_fill_b
    #define __msa_fill_h    __builtin_msa_fill_h
    #define __msa_fill_w    __builtin_msa_fill_w
    #define __msa_copy_u_d  __builtin_msa_copy_u_d
    #define __msa_insert_d  __builtin_msa_insert_d
    #define __msa_ldi_h     __builtin_msa_ldi_h

    #define CLTI_S_H(a, b) __msa_clti_s_h((v8i16)a, b)
#else
    #include <msa.h>

    #define CLTI_S_H(a, b) ((v8i16)a < b)
#endif

#define LD_V(RTYPE, psrc) *((RTYPE *)(psrc))
#define LD_UB(...) LD_V(v16u8, __VA_ARGS__)

#define ST_V(RTYPE, in, pdst) *((RTYPE *)(pdst)) = (in)
#define ST_UB(...) ST_V(v16u8, __VA_ARGS__)

#ifdef CLANG_BUILD
    #define LW(psrc)                             \
    ( {                                          \
        UWORD8 *psrc_lw_m = (UWORD8 *)(psrc);    \
        UWORD32 val_m;                           \
                                                 \
        asm volatile (                           \
            "lw  %[val_m],  %[psrc_lw_m]  \n\t"  \
                                                 \
            : [val_m] "=r" (val_m)               \
            : [psrc_lw_m] "m" (*psrc_lw_m)       \
        );                                       \
                                                 \
        val_m;                                   \
    } )

    #if (__mips == 64)
        #define LD(psrc)                             \
        ( {                                          \
            UWORD8 *psrc_ld_m = (UWORD8 *)(psrc);    \
            uint64_t val_m = 0;                      \
                                                     \
            asm volatile (                           \
                "ld  %[val_m],  %[psrc_ld_m]  \n\t"  \
                                                     \
                : [val_m] "=r" (val_m)               \
                : [psrc_ld_m] "m" (*psrc_ld_m)       \
            );                                       \
                                                     \
            val_m;                                   \
        } )
    #else  // !(__mips == 64)
        #define LD(psrc)                                             \
        ( {                                                          \
            UWORD8 *psrc_ld_m = (UWORD8 *)(psrc);                    \
            UWORD32 val0_m, val1_m;                                  \
            uint64_t val_m = 0;                                      \
                                                                     \
            val0_m = LW(psrc_ld_m);                                  \
            val1_m = LW(psrc_ld_m + 4);                              \
                                                                     \
            val_m = (uint64_t)(val1_m);                              \
            val_m = (uint64_t)((val_m << 32) & 0xFFFFFFFF00000000);  \
            val_m = (uint64_t)(val_m | (uint64_t)val0_m);            \
                                                                     \
            val_m;                                                   \
        } )
    #endif  // (__mips == 64)

    #define SW(val, pdst)                        \
    {                                            \
        UWORD8 *pdst_sw_m = (UWORD8 *)(pdst);    \
        UWORD32 val_m = (val);                   \
                                                 \
        asm volatile (                           \
            "sw  %[val_m],  %[pdst_sw_m]  \n\t"  \
                                                 \
            : [pdst_sw_m] "=m" (*pdst_sw_m)      \
            : [val_m] "r" (val_m)                \
        );                                       \
    }

    #if (__mips == 64)
        #define SD(val, pdst)                        \
        {                                            \
            UWORD8 *pdst_sd_m = (UWORD8 *)(pdst);    \
            uint64_t val_m = (val);                  \
                                                     \
            asm volatile (                           \
                "sd  %[val_m],  %[pdst_sd_m]  \n\t"  \
                                                     \
                : [pdst_sd_m] "=m" (*pdst_sd_m)      \
                : [val_m] "r" (val_m)                \
            );                                       \
        }
    #else
        #define SD(val, pdst)                                        \
        {                                                            \
            UWORD8 *pdst_sd_m = (UWORD8 *)(pdst);                    \
            UWORD32 val0_m, val1_m;                                  \
                                                                     \
            val0_m = (UWORD32)((val) & 0x00000000FFFFFFFF);          \
            val1_m = (UWORD32)(((val) >> 32) & 0x00000000FFFFFFFF);  \
                                                                     \
            SW(val0_m, pdst_sd_m);                                   \
            SW(val1_m, pdst_sd_m + 4);                               \
        }
    #endif
#else
#if (__mips_isa_rev >= 6)
    #define LW(psrc)                             \
    ( {                                          \
        UWORD8 *psrc_lw_m = (UWORD8 *)(psrc);    \
        UWORD32 val_m;                           \
                                                 \
        asm volatile (                           \
            "lw  %[val_m],  %[psrc_lw_m]  \n\t"  \
                                                 \
            : [val_m] "=r" (val_m)               \
            : [psrc_lw_m] "m" (*psrc_lw_m)       \
        );                                       \
                                                 \
        val_m;                                   \
    } )

    #if (__mips == 64)
        #define LD(psrc)                             \
        ( {                                          \
            UWORD8 *psrc_ld_m = (UWORD8 *)(psrc);    \
            uint64_t val_m = 0;                      \
                                                     \
            asm volatile (                           \
                "ld  %[val_m],  %[psrc_ld_m]  \n\t"  \
                                                     \
                : [val_m] "=r" (val_m)               \
                : [psrc_ld_m] "m" (*psrc_ld_m)       \
            );                                       \
                                                     \
            val_m;                                   \
        } )
    #else  // !(__mips == 64)
        #define LD(psrc)                                             \
        ( {                                                          \
            UWORD8 *psrc_ld_m = (UWORD8 *)(psrc);                    \
            UWORD32 val0_m, val1_m;                                  \
            uint64_t val_m = 0;                                      \
                                                                     \
            val0_m = LW(psrc_ld_m);                                  \
            val1_m = LW(psrc_ld_m + 4);                              \
                                                                     \
            val_m = (uint64_t)(val1_m);                              \
            val_m = (uint64_t)((val_m << 32) & 0xFFFFFFFF00000000);  \
            val_m = (uint64_t)(val_m | (uint64_t)val0_m);            \
                                                                     \
            val_m;                                                   \
        } )
    #endif  // (__mips == 64)

    #define SW(val, pdst)                        \
    {                                            \
        UWORD8 *pdst_sw_m = (UWORD8 *)(pdst);    \
        UWORD32 val_m = (val);                   \
                                                 \
        asm volatile (                           \
            "sw  %[val_m],  %[pdst_sw_m]  \n\t"  \
                                                 \
            : [pdst_sw_m] "=m" (*pdst_sw_m)      \
            : [val_m] "r" (val_m)                \
        );                                       \
    }

    #define SD(val, pdst)                        \
    {                                            \
        UWORD8 *pdst_sd_m = (UWORD8 *)(pdst);    \
        uint64_t val_m = (val);                  \
                                                 \
        asm volatile (                           \
            "sd  %[val_m],  %[pdst_sd_m]  \n\t"  \
                                                 \
            : [pdst_sd_m] "=m" (*pdst_sd_m)      \
            : [val_m] "r" (val_m)                \
        );                                       \
    }
#else  // !(__mips_isa_rev >= 6)
    #define LW(psrc)                              \
    ( {                                           \
        UWORD8 *psrc_lw_m = (UWORD8 *)(psrc);     \
        UWORD32 val_m;                            \
                                                  \
        asm volatile (                            \
            "ulw  %[val_m],  %[psrc_lw_m]  \n\t"  \
                                                  \
            : [val_m] "=r" (val_m)                \
            : [psrc_lw_m] "m" (*psrc_lw_m)        \
        );                                        \
                                                  \
        val_m;                                    \
    } )

    #if (__mips == 64)
        #define LD(psrc)                              \
        ( {                                           \
            UWORD8 *psrc_ld_m = (UWORD8 *)(psrc);     \
            uint64_t val_m = 0;                       \
                                                      \
            asm volatile (                            \
                "uld  %[val_m],  %[psrc_ld_m]  \n\t"  \
                                                      \
                : [val_m] "=r" (val_m)                \
                : [psrc_ld_m] "m" (*psrc_ld_m)        \
            );                                        \
                                                      \
            val_m;                                    \
        } )
    #else  // !(__mips == 64)
        #define LD(psrc)                                             \
        ( {                                                          \
            UWORD8 *psrc_ld_m = (UWORD8 *)(psrc);                    \
            UWORD32 val0_m, val1_m;                                  \
            uint64_t val_m = 0;                                      \
                                                                     \
            val0_m = LW(psrc_ld_m);                                  \
            val1_m = LW(psrc_ld_m + 4);                              \
                                                                     \
            val_m = (uint64_t)(val1_m);                              \
            val_m = (uint64_t)((val_m << 32) & 0xFFFFFFFF00000000);  \
            val_m = (uint64_t)(val_m | (uint64_t)val0_m);            \
                                                                     \
            val_m;                                                   \
        } )
    #endif  // (__mips == 64)

    #define SW(val, pdst)                         \
    {                                             \
        UWORD8 *pdst_sw_m = (UWORD8 *)(pdst);     \
        UWORD32 val_m = (val);                    \
                                                  \
        asm volatile (                            \
            "usw  %[val_m],  %[pdst_sw_m]  \n\t"  \
                                                  \
            : [pdst_sw_m] "=m" (*pdst_sw_m)       \
            : [val_m] "r" (val_m)                 \
        );                                        \
    }

    #define SD(val, pdst)                                        \
    {                                                            \
        UWORD8 *pdst_sd_m = (UWORD8 *)(pdst);                    \
        UWORD32 val0_m, val1_m;                                  \
                                                                 \
        val0_m = (UWORD32)((val) & 0x00000000FFFFFFFF);          \
        val1_m = (UWORD32)(((val) >> 32) & 0x00000000FFFFFFFF);  \
                                                                 \
        SW(val0_m, pdst_sd_m);                                   \
        SW(val1_m, pdst_sd_m + 4);                               \
    }
#endif  // (__mips_isa_rev >= 6)
#endif  // #ifdef CLANG_BUILD

/* Description : Load double words with stride
   Arguments   : Inputs  - psrc, stride
                 Outputs - out0, out1
   Details     : Load double word in 'out0' from (psrc)
                 Load double word in 'out1' from (psrc + stride)
*/
#define LD2(psrc, stride, out0, out1)  \
{                                      \
    out0 = LD((psrc));                 \
    out1 = LD((psrc) + stride);        \
}
#define LD4(psrc, stride, out0, out1, out2, out3)  \
{                                                  \
    LD2((psrc), stride, out0, out1);               \
    LD2((psrc) + 2 * stride, stride, out2, out3);  \
}

/* Description : Store 4 double words with stride
   Arguments   : Inputs - in0, in1, in2, in3, pdst, stride
   Details     : Store double word from 'in0' to (pdst)
                 Store double word from 'in1' to (pdst + stride)
                 Store double word from 'in2' to (pdst + 2 * stride)
                 Store double word from 'in3' to (pdst + 3 * stride)
*/
#define SD4(in0, in1, in2, in3, pdst, stride)  \
{                                              \
    SD(in0, (pdst));                           \
    SD(in1, (pdst) + stride);                  \
    SD(in2, (pdst) + 2 * stride);              \
    SD(in3, (pdst) + 3 * stride);              \
}

/* Description : Load vectors with 16 byte elements with stride
   Arguments   : Inputs  - psrc, stride
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Load 16 byte elements in 'out0' from (psrc)
                 Load 16 byte elements in 'out1' from (psrc + stride)
*/
#define LD_V2(RTYPE, psrc, stride, out0, out1)  \
{                                               \
    out0 = LD_V(RTYPE, (psrc));                 \
    out1 = LD_V(RTYPE, (psrc) + stride);        \
}

#define LD_V4(RTYPE, psrc, stride, out0, out1, out2, out3)   \
{                                                            \
    LD_V2(RTYPE, (psrc), stride, out0, out1);                \
    LD_V2(RTYPE, (psrc) + 2 * stride , stride, out2, out3);  \
}
#define LD_UB4(...) LD_V4(v16u8, __VA_ARGS__)
#define LD_SH4(...) LD_V4(v8i16, __VA_ARGS__)

#define LD_V8(RTYPE, psrc, stride,                                      \
              out0, out1, out2, out3, out4, out5, out6, out7)           \
{                                                                       \
    LD_V4(RTYPE, (psrc), stride, out0, out1, out2, out3);               \
    LD_V4(RTYPE, (psrc) + 4 * stride, stride, out4, out5, out6, out7);  \
}
#define LD_UB8(...) LD_V8(v16u8, __VA_ARGS__)
#define LD_SH8(...) LD_V8(v8i16, __VA_ARGS__)

/* Description : Store vectors of 16 byte elements with stride
   Arguments   : Inputs - in0, in1, pdst, stride
   Details     : Store 16 byte elements from 'in0' to (pdst)
                 Store 16 byte elements from 'in1' to (pdst + stride)
*/
#define ST_V2(RTYPE, in0, in1, pdst, stride)  \
{                                             \
    ST_V(RTYPE, in0, (pdst));                 \
    ST_V(RTYPE, in1, (pdst) + stride);        \
}
#define ST_UB2(...) ST_V2(v16u8, __VA_ARGS__)

#define ST_V4(RTYPE, in0, in1, in2, in3, pdst, stride)    \
{                                                         \
    ST_V2(RTYPE, in0, in1, (pdst), stride);               \
    ST_V2(RTYPE, in2, in3, (pdst) + 2 * stride, stride);  \
}
#define ST_UB4(...) ST_V4(v16u8, __VA_ARGS__)
#define ST_SB4(...) ST_V4(v16i8, __VA_ARGS__)

#define ST_V8(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,        \
              pdst, stride)                                         \
{                                                                   \
    ST_V4(RTYPE, in0, in1, in2, in3, pdst, stride);                 \
    ST_V4(RTYPE, in4, in5, in6, in7, (pdst) + 4 * stride, stride);  \
}
#define ST_UB8(...) ST_V8(v16u8, __VA_ARGS__)
#define ST_UH8(...) ST_V8(v8u16, __VA_ARGS__)

/* Description : Store 8x4 byte block to destination memory from input
                 vectors
   Arguments   : Inputs - in0, in1, pdst, stride
   Details     : Index 0 double word element from 'in0' vector is copied to the
                 GP register and stored to (pdst)
                 Index 1 double word element from 'in0' vector is copied to the
                 GP register and stored to (pdst + stride)
                 Index 0 double word element from 'in1' vector is copied to the
                 GP register and stored to (pdst + 2 * stride)
                 Index 1 double word element from 'in1' vector is copied to the
                 GP register and stored to (pdst + 3 * stride)
*/
#define ST8x4_UB(in0, in1, pdst, stride)                      \
{                                                             \
    uint64_t out0_m, out1_m, out2_m, out3_m;                  \
    UWORD8 *pblk_8x4_m = (UWORD8 *)(pdst);                    \
                                                              \
    out0_m = __msa_copy_u_d((v2i64)in0, 0);                   \
    out1_m = __msa_copy_u_d((v2i64)in0, 1);                   \
    out2_m = __msa_copy_u_d((v2i64)in1, 0);                   \
    out3_m = __msa_copy_u_d((v2i64)in1, 1);                   \
                                                              \
    SD4(out0_m, out1_m, out2_m, out3_m, pblk_8x4_m, stride);  \
}

/* Description : average with rounding (in0 + in1 + 1) / 2.
   Arguments   : Inputs  - in0, in1, in2, in3,
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Each unsigned byte element from 'in0' vector is added with
                 each unsigned byte element from 'in1' vector. Then the average
                 with rounding is calculated and written to 'out0'
*/
#define AVER_UB2(RTYPE, in0, in1, in2, in3, out0, out1)    \
{                                                          \
    out0 = (RTYPE)__msa_aver_u_b((v16u8)in0, (v16u8)in1);  \
    out1 = (RTYPE)__msa_aver_u_b((v16u8)in2, (v16u8)in3);  \
}
#define AVER_UB2_UB(...) AVER_UB2(v16u8, __VA_ARGS__)

#define AVER_UB4(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7, \
                 out0, out1, out2, out3)                        \
{                                                               \
    AVER_UB2(RTYPE, in0, in1, in2, in3, out0, out1);            \
    AVER_UB2(RTYPE, in4, in5, in6, in7, out2, out3);            \
}
#define AVER_UB4_UB(...) AVER_UB4(v16u8, __VA_ARGS__)

/* Description : Immediate number of elements to slide with zero
   Arguments   : Inputs  - in0, in1, slide_val
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Byte elements from 'zero_m' vector are slid into 'in0' by
                 value specified in the 'slide_val'
*/
#define SLDI_B2_0(RTYPE, in0, in1, out0, out1, slide_val)              \
{                                                                      \
    v16i8 zero_m = { 0 };                                              \
    out0 = (RTYPE)__msa_sldi_b((v16i8)zero_m, (v16i8)in0, slide_val);  \
    out1 = (RTYPE)__msa_sldi_b((v16i8)zero_m, (v16i8)in1, slide_val);  \
}

#define SLDI_B4_0(RTYPE, in0, in1, in2, in3,            \
                  out0, out1, out2, out3, slide_val)    \
{                                                       \
    SLDI_B2_0(RTYPE, in0, in1, out0, out1, slide_val);  \
    SLDI_B2_0(RTYPE, in2, in3, out2, out3, slide_val);  \
}
#define SLDI_B4_0_UB(...) SLDI_B4_0(v16u8, __VA_ARGS__)

/* Description : Immediate number of elements to slide
   Arguments   : Inputs  - in0_0, in0_1, in1_0, in1_1, slide_val
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Byte elements from 'in0_0' vector are slid into 'in1_0' by
                 value specified in the 'slide_val'
*/
#define SLDI_B2(RTYPE, in0_0, in0_1, in1_0, in1_1, out0, out1, slide_val)  \
{                                                                          \
    out0 = (RTYPE)__msa_sldi_b((v16i8)in0_0, (v16i8)in1_0, slide_val);     \
    out1 = (RTYPE)__msa_sldi_b((v16i8)in0_1, (v16i8)in1_1, slide_val);     \
}
#define SLDI_B2_UB(...) SLDI_B2(v16u8, __VA_ARGS__)

/* Description : Dot product of halfword vector elements
   Arguments   : Inputs  - mult0, mult1, cnst0, cnst1
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Signed halfword elements from 'mult0' are multiplied with
                 signed halfword elements from 'cnst0' producing a result
                 twice the size of input i.e. signed word.
                 The multiplication result of adjacent odd-even elements
                 are added together and written to the 'out0' vector
*/
#define DOTP_SH2(RTYPE, mult0, mult1, cnst0, cnst1, out0, out1)  \
{                                                                \
    out0 = (RTYPE)__msa_dotp_s_w((v8i16)mult0, (v8i16)cnst0);    \
    out1 = (RTYPE)__msa_dotp_s_w((v8i16)mult1, (v8i16)cnst1);    \
}
#define DOTP_SH2_SW(...) DOTP_SH2(v4i32, __VA_ARGS__)

#define DOTP_SH4(RTYPE, mult0, mult1, mult2, mult3,           \
                 cnst0, cnst1, cnst2, cnst3,                  \
                 out0, out1, out2, out3)                      \
{                                                             \
    DOTP_SH2(RTYPE, mult0, mult1, cnst0, cnst1, out0, out1);  \
    DOTP_SH2(RTYPE, mult2, mult3, cnst2, cnst3, out2, out3);  \
}
#define DOTP_SH4_SW(...) DOTP_SH4(v4i32, __VA_ARGS__)

/* Description : Dot product & addition of halfword vector elements
   Arguments   : Inputs  - mult0, mult1, cnst0, cnst1
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Signed halfword elements from 'mult0' are multiplied with
                 signed halfword elements from 'cnst0' producing a result
                 twice the size of input i.e. signed word.
                 The multiplication result of adjacent odd-even elements
                 are added to the 'out0' vector
*/
#define DPADD_SH2(RTYPE, mult0, mult1, cnst0, cnst1, out0, out1)             \
{                                                                            \
    out0 = (RTYPE)__msa_dpadd_s_w((v4i32)out0, (v8i16)mult0, (v8i16)cnst0);  \
    out1 = (RTYPE)__msa_dpadd_s_w((v4i32)out1, (v8i16)mult1, (v8i16)cnst1);  \
}

#define DPADD_SH4(RTYPE, mult0, mult1, mult2, mult3,                   \
                  cnst0, cnst1, cnst2, cnst3, out0, out1, out2, out3)  \
{                                                                      \
    DPADD_SH2(RTYPE, mult0, mult1, cnst0, cnst1, out0, out1);          \
    DPADD_SH2(RTYPE, mult2, mult3, cnst2, cnst3, out2, out3);          \
}
#define DPADD_SH4_SW(...) DPADD_SH4(v4i32, __VA_ARGS__)

/* Description : Clips all signed halfword elements of input vector
                 between 0 & 255
   Arguments   : Input  - in
                 Output - out_m
                 Return Type - signed halfword
*/
#define CLIP_SH_0_255(in)                               \
( {                                                     \
    v8i16 max_m = __msa_ldi_h(255);                     \
    v8i16 out_m;                                        \
                                                        \
    out_m = __msa_maxi_s_h((v8i16)in, 0);               \
    out_m = __msa_min_s_h((v8i16)max_m, (v8i16)out_m);  \
    out_m;                                              \
} )
#define CLIP_SH2_0_255(in0, in1)  \
{                                 \
    in0 = CLIP_SH_0_255(in0);     \
    in1 = CLIP_SH_0_255(in1);     \
}
#define CLIP_SH4_0_255(in0, in1, in2, in3)  \
{                                           \
    CLIP_SH2_0_255(in0, in1);               \
    CLIP_SH2_0_255(in2, in3);               \
}

/* Description : Horizontal addition of unsigned byte vector elements
   Arguments   : Inputs  - in0, in1
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Each unsigned odd byte element from 'in0' is added to
                 even unsigned byte element from 'in0' (pairwise) and the
                 halfword result is written to 'out0'
*/
#define HADD_UB2(RTYPE, in0, in1, out0, out1)              \
{                                                          \
    out0 = (RTYPE)__msa_hadd_u_h((v16u8)in0, (v16u8)in0);  \
    out1 = (RTYPE)__msa_hadd_u_h((v16u8)in1, (v16u8)in1);  \
}

#define HADD_UB4(RTYPE, in0, in1, in2, in3, out0, out1, out2, out3)  \
{                                                                    \
    HADD_UB2(RTYPE, in0, in1, out0, out1);                           \
    HADD_UB2(RTYPE, in2, in3, out2, out3);                           \
}
#define HADD_UB4_UH(...) HADD_UB4(v8u16, __VA_ARGS__)

/* Description : Set element n input vector to GPR value
   Arguments   : Inputs - in0, in1, in2, in3
                 Output - out
                 Return Type - as per RTYPE
   Details     : Set element 0 in vector 'out' to value specified in 'in0'
*/
#define INSERT_D2(RTYPE, in0, in1, out)               \
{                                                     \
    out = (RTYPE)__msa_insert_d((v2i64)out, 0, in0);  \
    out = (RTYPE)__msa_insert_d((v2i64)out, 1, in1);  \
}
#define INSERT_D2_UB(...) INSERT_D2(v16u8, __VA_ARGS__)

/* Description : Interleave left half of byte elements from vectors
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Left half of byte elements of 'in0' and 'in1' are interleaved
                 and written to 'out0'.
*/
#define ILVL_B2(RTYPE, in0, in1, in2, in3, out0, out1)   \
{                                                        \
    out0 = (RTYPE)__msa_ilvl_b((v16i8)in0, (v16i8)in1);  \
    out1 = (RTYPE)__msa_ilvl_b((v16i8)in2, (v16i8)in3);  \
}

#define ILVL_B4(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,  \
                out0, out1, out2, out3)                         \
{                                                               \
    ILVL_B2(RTYPE, in0, in1, in2, in3, out0, out1);             \
    ILVL_B2(RTYPE, in4, in5, in6, in7, out2, out3);             \
}
#define ILVL_B4_UB(...) ILVL_B4(v16u8, __VA_ARGS__)
#define ILVL_B4_UH(...) ILVL_B4(v8u16, __VA_ARGS__)

/* Description : Interleave left half of halfword elements from vectors
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Left half of halfword elements of 'in0' and 'in1' are
                 interleaved and written to 'out0'.
*/
#define ILVL_H2(RTYPE, in0, in1, in2, in3, out0, out1)   \
{                                                        \
    out0 = (RTYPE)__msa_ilvl_h((v8i16)in0, (v8i16)in1);  \
    out1 = (RTYPE)__msa_ilvl_h((v8i16)in2, (v8i16)in3);  \
}
#define ILVL_H2_SH(...) ILVL_H2(v8i16, __VA_ARGS__)

/* Description : Interleave right half of byte elements from vectors
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Right half of byte elements of 'in0' and 'in1' are interleaved
                 and written to out0.
*/
#define ILVR_B2(RTYPE, in0, in1, in2, in3, out0, out1)   \
{                                                        \
    out0 = (RTYPE)__msa_ilvr_b((v16i8)in0, (v16i8)in1);  \
    out1 = (RTYPE)__msa_ilvr_b((v16i8)in2, (v16i8)in3);  \
}

#define ILVR_B4(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,  \
                out0, out1, out2, out3)                         \
{                                                               \
    ILVR_B2(RTYPE, in0, in1, in2, in3, out0, out1);             \
    ILVR_B2(RTYPE, in4, in5, in6, in7, out2, out3);             \
}
#define ILVR_B4_UB(...) ILVR_B4(v16u8, __VA_ARGS__)
#define ILVR_B4_SH(...) ILVR_B4(v8i16, __VA_ARGS__)

#define ILVR_B8(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,    \
                in8, in9, in10, in11, in12, in13, in14, in15,     \
                out0, out1, out2, out3, out4, out5, out6, out7)   \
{                                                                 \
    ILVR_B4(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,        \
            out0, out1, out2, out3);                              \
    ILVR_B4(RTYPE, in8, in9, in10, in11, in12, in13, in14, in15,  \
            out4, out5, out6, out7);                              \
}
#define ILVR_B8_UH(...) ILVR_B8(v8u16, __VA_ARGS__)

/* Description : Interleave right half of halfword elements from vectors
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Right half of halfword elements of 'in0' and 'in1' are
                 interleaved and written to 'out0'.
*/
#define ILVR_H2(RTYPE, in0, in1, in2, in3, out0, out1)   \
{                                                        \
    out0 = (RTYPE)__msa_ilvr_h((v8i16)in0, (v8i16)in1);  \
    out1 = (RTYPE)__msa_ilvr_h((v8i16)in2, (v8i16)in3);  \
}
#define ILVR_H2_SH(...) ILVR_H2(v8i16, __VA_ARGS__)

#define ILVR_H4(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,  \
                out0, out1, out2, out3)                         \
{                                                               \
    ILVR_H2(RTYPE, in0, in1, in2, in3, out0, out1);             \
    ILVR_H2(RTYPE, in4, in5, in6, in7, out2, out3);             \
}
#define ILVR_H4_SH(...) ILVR_H4(v8i16, __VA_ARGS__)

/* Description : Interleave both left and right half of input vectors
   Arguments   : Inputs  - in0, in1
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Right half of byte elements from 'in0' and 'in1' are
                 interleaved and written to 'out0'
*/
#define ILVRL_B2(RTYPE, in0, in1, out0, out1)            \
{                                                        \
    out0 = (RTYPE)__msa_ilvr_b((v16i8)in0, (v16i8)in1);  \
    out1 = (RTYPE)__msa_ilvl_b((v16i8)in0, (v16i8)in1);  \
}
#define ILVRL_B2_UB(...) ILVRL_B2(v16u8, __VA_ARGS__)

#define ILVRL_H2(RTYPE, in0, in1, out0, out1)            \
{                                                        \
    out0 = (RTYPE)__msa_ilvr_h((v8i16)in0, (v8i16)in1);  \
    out1 = (RTYPE)__msa_ilvl_h((v8i16)in0, (v8i16)in1);  \
}
#define ILVRL_H2_SH(...) ILVRL_H2(v8i16, __VA_ARGS__)
#define ILVRL_H2_SW(...) ILVRL_H2(v4i32, __VA_ARGS__)

#define ILVRL_W2(RTYPE, in0, in1, out0, out1)            \
{                                                        \
    out0 = (RTYPE)__msa_ilvr_w((v4i32)in0, (v4i32)in1);  \
    out1 = (RTYPE)__msa_ilvl_w((v4i32)in0, (v4i32)in1);  \
}
#define ILVRL_W2_SH(...) ILVRL_W2(v8i16, __VA_ARGS__)

#define ILVRL_D2(RTYPE, in0, in1, out0, out1)            \
{                                                        \
    out0 = (RTYPE)__msa_ilvr_d((v2i64)in0, (v2i64)in1);  \
    out1 = (RTYPE)__msa_ilvl_d((v2i64)in0, (v2i64)in1);  \
}
#define ILVRL_D2_SH(...) ILVRL_D2(v8i16, __VA_ARGS__)

/* Description : Pack even byte elements of vector pairs
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Even byte elements of 'in0' are copied to the left half of
                 'out0' & even byte elements of 'in1' are copied to the right
                 half of 'out0'.
*/
#define PCKEV_B2(RTYPE, in0, in1, in2, in3, out0, out1)   \
{                                                         \
    out0 = (RTYPE)__msa_pckev_b((v16i8)in0, (v16i8)in1);  \
    out1 = (RTYPE)__msa_pckev_b((v16i8)in2, (v16i8)in3);  \
}
#define PCKEV_B2_SB(...) PCKEV_B2(v16i8, __VA_ARGS__)
#define PCKEV_B2_UB(...) PCKEV_B2(v16u8, __VA_ARGS__)

#define PCKEV_B4(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,  \
                 out0, out1, out2, out3)                         \
{                                                                \
    PCKEV_B2(RTYPE, in0, in1, in2, in3, out0, out1);             \
    PCKEV_B2(RTYPE, in4, in5, in6, in7, out2, out3);             \
}
#define PCKEV_B4_UB(...) PCKEV_B4(v16u8, __VA_ARGS__)
#define PCKEV_B4_SH(...) PCKEV_B4(v8i16, __VA_ARGS__)

/* Description : Pack even halfword elements of vector pairs
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Even halfword elements of 'in0' are copied to the left half of
                 'out0' & even halfword elements of 'in1' are copied to the
                 right half of 'out0'.
*/
#define PCKEV_H2(RTYPE, in0, in1, in2, in3, out0, out1)   \
{                                                         \
    out0 = (RTYPE)__msa_pckev_h((v8i16)in0, (v8i16)in1);  \
    out1 = (RTYPE)__msa_pckev_h((v8i16)in2, (v8i16)in3);  \
}

#define PCKEV_H4(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,  \
                 out0, out1, out2, out3)                         \
{                                                                \
    PCKEV_H2(RTYPE, in0, in1, in2, in3, out0, out1);             \
    PCKEV_H2(RTYPE, in4, in5, in6, in7, out2, out3);             \
}
#define PCKEV_H4_SH(...) PCKEV_H4(v8i16, __VA_ARGS__)

/* Description : Pack even double word elements of vector pairs
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Even double elements of 'in0' are copied to the left half of
                 'out0' & even double elements of 'in1' are copied to the right
                 half of 'out0'.
*/
#define PCKEV_D2(RTYPE, in0, in1, in2, in3, out0, out1)   \
{                                                         \
    out0 = (RTYPE)__msa_pckev_d((v2i64)in0, (v2i64)in1);  \
    out1 = (RTYPE)__msa_pckev_d((v2i64)in2, (v2i64)in3);  \
}

#define PCKEV_D4(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,  \
                 out0, out1, out2, out3)                         \
{                                                                \
    PCKEV_D2(RTYPE, in0, in1, in2, in3, out0, out1);             \
    PCKEV_D2(RTYPE, in4, in5, in6, in7, out2, out3);             \
}

/* Description : Pack odd double word elements of vector pairs
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
                 Return Type - as per RTYPE
   Details     : Odd double word elements of 'in0' are copied to the left half
                 of 'out0' & odd double word elements of 'in1' are copied to
                 the right half of 'out0'.
*/
#define PCKOD_D2(RTYPE, in0, in1, in2, in3, out0, out1)   \
{                                                         \
    out0 = (RTYPE)__msa_pckod_d((v2i64)in0, (v2i64)in1);  \
    out1 = (RTYPE)__msa_pckod_d((v2i64)in2, (v2i64)in3);  \
}

/* Description : Shift right arithmetic rounded (immediate)
   Arguments   : Inputs  - in0, in1, shift
                 Outputs - in place operation
                 Return Type - as per RTYPE
   Details     : Each element of vector 'in0' is shifted right arithmetically by
                 the value in 'shift'. The last discarded bit is added to the
                 shifted value for rounding and the result is written in-place.
                 'shift' is an immediate value.
*/
#define SRARI_H2(RTYPE, in0, in1, shift)            \
{                                                   \
    in0 = (RTYPE)__msa_srari_h((v8i16)in0, shift);  \
    in1 = (RTYPE)__msa_srari_h((v8i16)in1, shift);  \
}

#define SRARI_H4(RTYPE, in0, in1, in2, in3, shift)    \
{                                                     \
    SRARI_H2(RTYPE, in0, in1, shift);                 \
    SRARI_H2(RTYPE, in2, in3, shift);                 \
}
#define SRARI_H4_UH(...) SRARI_H4(v8u16, __VA_ARGS__)

#define SRARI_W2(RTYPE, in0, in1, shift)            \
{                                                   \
    in0 = (RTYPE)__msa_srari_w((v4i32)in0, shift);  \
    in1 = (RTYPE)__msa_srari_w((v4i32)in1, shift);  \
}

#define SRARI_W4(RTYPE, in0, in1, in2, in3, shift)  \
{                                                   \
    SRARI_W2(RTYPE, in0, in1, shift);               \
    SRARI_W2(RTYPE, in2, in3, shift);               \
}
#define SRARI_W4_SW(...) SRARI_W4(v4i32, __VA_ARGS__)

/* Description : Addition of 2 pairs of vectors
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
   Details     : Each element in 'in0' is added to 'in1' and result is written
                 to 'out0'.
*/
#define ADD2(in0, in1, in2, in3, out0, out1)  \
{                                             \
    out0 = in0 + in1;                         \
    out1 = in2 + in3;                         \
}
#define ADD4(in0, in1, in2, in3, in4, in5, in6, in7,  \
             out0, out1, out2, out3)                  \
{                                                     \
    ADD2(in0, in1, in2, in3, out0, out1);             \
    ADD2(in4, in5, in6, in7, out2, out3);             \
}

/* Description : Subtraction of 2 pairs of vectors
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1
   Details     : Each element in 'in1' is subtracted from 'in0' and result is
                 written to 'out0'.
*/
#define SUB4(in0, in1, in2, in3, in4, in5, in6, in7,  \
             out0, out1, out2, out3)                  \
{                                                     \
    out0 = in0 - in1;                                 \
    out1 = in2 - in3;                                 \
    out2 = in4 - in5;                                 \
    out3 = in6 - in7;                                 \
}

/* Description : Sign extend halfword elements from input vector and return
                 the result in pair of vectors
   Arguments   : Input   - in            (halfword vector)
                 Outputs - out0, out1   (sign extended word vectors)
                 Return Type - signed word
   Details     : Sign bit of halfword elements from input vector 'in' is
                 extracted and interleaved right with same vector 'in0' to
                 generate 4 signed word elements in 'out0'
                 Then interleaved left with same vector 'in0' to
                 generate 4 signed word elements in 'out1'
*/
#define UNPCK_SH_SW(in, out0, out1)      \
{                                        \
    v8i16 tmp_m;                         \
                                         \
    tmp_m = CLTI_S_H((v8i16)in, 0);      \
    ILVRL_H2_SW(tmp_m, in, out0, out1);  \
}

/* Description : Butterfly of 4 input vectors
   Arguments   : Inputs  - in0, in1, in2, in3
                 Outputs - out0, out1, out2, out3
   Details     : Butterfly operation
*/
#define BUTTERFLY_4(in0, in1, in2, in3, out0, out1, out2, out3)  \
{                                                                \
    out0 = in0 + in3;                                            \
    out1 = in1 + in2;                                            \
                                                                 \
    out2 = in1 - in2;                                            \
    out3 = in0 - in3;                                            \
}

/* Description : Butterfly of 8 input vectors
   Arguments   : Inputs  - in0 ...  in7
                 Outputs - out0 .. out7
   Details     : Butterfly operation
*/
#define BUTTERFLY_8(in0, in1, in2, in3, in4, in5, in6, in7,          \
                    out0, out1, out2, out3, out4, out5, out6, out7)  \
{                                                                    \
    out0 = in0 + in7;                                                \
    out1 = in1 + in6;                                                \
    out2 = in2 + in5;                                                \
    out3 = in3 + in4;                                                \
                                                                     \
    out4 = in3 - in4;                                                \
    out5 = in2 - in5;                                                \
    out6 = in1 - in6;                                                \
    out7 = in0 - in7;                                                \
}

/* Description : Transpose 4x8 block with half word elements in vectors
   Arguments   : Inputs  - in0, in1, in2, in3, in4, in5, in6, in7
                 Outputs - out0, out1, out2, out3, out4, out5, out6, out7
                 Return Type - signed halfword
*/
#define TRANSPOSE4X8_SH_SH(in0, in1, in2, in3, in4, in5, in6, in7, out0,  \
                           out1, out2, out3, out4, out5, out6, out7)      \
{                                                                         \
    v8i16 tmp0_m, tmp1_m, tmp2_m, tmp3_m;                                 \
    v8i16 tmp0_n, tmp1_n, tmp2_n, tmp3_n;                                 \
    v8i16 zero_m = { 0 };                                                 \
                                                                          \
    ILVR_H4_SH(in1, in0, in3, in2, in5, in4, in7, in6,                    \
               tmp0_n, tmp1_n, tmp2_n, tmp3_n);                           \
    ILVRL_W2_SH(tmp1_n, tmp0_n, tmp0_m, tmp2_m);                          \
    ILVRL_W2_SH(tmp3_n, tmp2_n, tmp1_m, tmp3_m);                          \
    ILVRL_D2_SH(tmp1_m, tmp0_m, out0, out1);                              \
    ILVRL_D2_SH(tmp3_m, tmp2_m, out2, out3);                              \
                                                                          \
    out4 = zero_m;                                                        \
    out5 = zero_m;                                                        \
    out6 = zero_m;                                                        \
    out7 = zero_m;                                                        \
}

/* Description : Transpose 8x8 block with half word elements in vectors
   Arguments   : Inputs  - in0, in1, in2, in3, in4, in5, in6, in7
                 Outputs - out0, out1, out2, out3, out4, out5, out6, out7
                 Return Type - as per RTYPE
*/
#define TRANSPOSE8x8_H(RTYPE, in0, in1, in2, in3, in4, in5, in6, in7,   \
                       out0, out1, out2, out3, out4, out5, out6, out7)  \
{                                                                       \
    v8i16 s0_m, s1_m;                                                   \
    v8i16 tmp0_m, tmp1_m, tmp2_m, tmp3_m;                               \
    v8i16 tmp4_m, tmp5_m, tmp6_m, tmp7_m;                               \
                                                                        \
    ILVR_H2_SH(in6, in4, in7, in5, s0_m, s1_m);                         \
    ILVRL_H2_SH(s1_m, s0_m, tmp0_m, tmp1_m);                            \
    ILVL_H2_SH(in6, in4, in7, in5, s0_m, s1_m);                         \
    ILVRL_H2_SH(s1_m, s0_m, tmp2_m, tmp3_m);                            \
    ILVR_H2_SH(in2, in0, in3, in1, s0_m, s1_m);                         \
    ILVRL_H2_SH(s1_m, s0_m, tmp4_m, tmp5_m);                            \
    ILVL_H2_SH(in2, in0, in3, in1, s0_m, s1_m);                         \
    ILVRL_H2_SH(s1_m, s0_m, tmp6_m, tmp7_m);                            \
    PCKEV_D4(RTYPE, tmp0_m, tmp4_m, tmp1_m, tmp5_m, tmp2_m, tmp6_m,     \
             tmp3_m, tmp7_m, out0, out2, out4, out6);                   \
    PCKOD_D2(RTYPE, tmp0_m, tmp4_m, tmp1_m, tmp5_m, out1, out3);        \
    PCKOD_D2(RTYPE, tmp2_m, tmp6_m, tmp3_m, tmp7_m, out5, out7);        \
}
#define TRANSPOSE8x8_SH_SH(...) TRANSPOSE8x8_H(v8i16, __VA_ARGS__)

/* Description : Average rounded byte elements from pair of vectors and store
                 8x4 byte block in destination memory
   Arguments   : Inputs - in0, in1, in2, in3, in4, in5, in6, in7, pdst, stride
   Details     : Each byte element from input vector pair 'in0' and 'in1' are
                 average rounded (a + b + 1) / 2 and stored in 'tmp0_m'
                 Each byte element from input vector pair 'in2' and 'in3' are
                 average rounded (a + b + 1) / 2 and stored in 'tmp1_m'
                 Each byte element from input vector pair 'in4' and 'in5' are
                 average rounded (a + b + 1) / 2 and stored in 'tmp2_m'
                 Each byte element from input vector pair 'in6' and 'in7' are
                 average rounded (a + b + 1) / 2 and stored in 'tmp3_m'
                 The half vector results from all 4 vectors are stored in
                 destination memory as 8x4 byte block
*/
#define AVER_ST8x4_UB(in0, in1, in2, in3, in4, in5, in6, in7, pdst, stride)  \
{                                                                            \
    uint64_t out0_m, out1_m, out2_m, out3_m;                                 \
    v16u8 tp0_m, tp1_m, tp2_m, tp3_m;                                        \
                                                                             \
    AVER_UB4_UB(in0, in1, in2, in3, in4, in5, in6, in7,                      \
                tp0_m, tp1_m, tp2_m, tp3_m);                                 \
                                                                             \
    out0_m = __msa_copy_u_d((v2i64)tp0_m, 0);                                \
    out1_m = __msa_copy_u_d((v2i64)tp1_m, 0);                                \
    out2_m = __msa_copy_u_d((v2i64)tp2_m, 0);                                \
    out3_m = __msa_copy_u_d((v2i64)tp3_m, 0);                                \
    SD4(out0_m, out1_m, out2_m, out3_m, pdst, stride);                       \
}

/* Description : Average rounded byte elements from pair of vectors and store
                 16x4 byte block in destination memory
   Arguments   : Inputs - in0, in1, in2, in3, in4, in5, in6, in7, pdst, stride
   Details     : Each byte element from input vector pair 'in0' and 'in1' are
                 average rounded (a + b + 1) / 2 and stored in 'tmp0_m'
                 Each byte element from input vector pair 'in2' and 'in3' are
                 average rounded (a + b + 1) / 2 and stored in 'tmp1_m'
                 Each byte element from input vector pair 'in4' and 'in5' are
                 average rounded (a + b + 1) / 2 and stored in 'tmp2_m'
                 Each byte element from input vector pair 'in6' and 'in7' are
                 average rounded (a + b + 1) / 2 and stored in 'tmp3_m'
                 The vector results from all 4 vectors are stored in
                 destination memory as 16x4 byte block
*/
#define AVER_ST16x4_UB(in0, in1, in2, in3, in4, in5, in6, in7, pdst, stride)  \
{                                                                             \
    v16u8 t0_m, t1_m, t2_m, t3_m;                                             \
                                                                              \
    AVER_UB4_UB(in0, in1, in2, in3, in4, in5, in6, in7,                       \
                t0_m, t1_m, t2_m, t3_m);                                      \
    ST_UB4(t0_m, t1_m, t2_m, t3_m, pdst, stride);                             \
}
#endif  /* __IMPEG2_MACROS_MSA_H__ */
