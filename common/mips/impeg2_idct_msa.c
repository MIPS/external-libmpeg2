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

#include "iv_datatypedef.h"
#include "impeg2_defs.h"
#include "impeg2_macros.h"
#include "impeg2_globals.h"
#include "impeg2_macros_msa.h"

#define MPEG2_SET_COSPI_PAIR(c0_h, c1_h)  \
( {                                       \
    v8i16 out0_m, r0_m, r1_m;             \
                                          \
    r0_m = __msa_fill_h(c0_h);            \
    r1_m = __msa_fill_h(c1_h);            \
    out0_m = __msa_ilvev_h(r1_m, r0_m);   \
                                          \
    out0_m;                               \
} )

static void mpeg2_idct_recon_dc_add_msa(WORD16 src, UWORD8 *pred,
                                        WORD32 pred_stride,
                                        const WORD16 *idct_coeff_q15,
                                        const WORD16 *idct_coeff_q11,
                                        const WORD16 *mismatch_coeff_additive,
                                        UWORD8 *dst, WORD32 dst_stride)
{
    const WORD16 *ptr_temp = mismatch_coeff_additive;
    WORD32 val, itr;
    v16u8 src0, src1, src2, src3;
    v8i16 src0_h, src1_h, src2_h, src3_h, temp0_h, temp1_h, temp2_h, temp3_h;
    v16i8 zero = {0};
    v4i32 temp0_w, temp1_w, temp2_w, temp3_w, temp4_w, temp5_w, temp6_w;
    v4i32 temp7_w, vec;

    val = src * idct_coeff_q15[0];
    val = ((val + ((1 << 12) >> 1)) >> 12);
    val *= idct_coeff_q11[0];
    vec = __msa_fill_w(val);

    for(itr = 2; itr--;)
    {
        LD_SH4(ptr_temp, 8, temp0_h, temp1_h, temp2_h, temp3_h);
        LD_UB4(pred, pred_stride, src0, src1, src2, src3);
        ILVR_B4_SH(zero, src0, zero, src1, zero, src2, zero, src3,
                   src0_h, src1_h, src2_h, src3_h);
        UNPCK_SH_SW(temp0_h, temp0_w, temp4_w);
        UNPCK_SH_SW(temp1_h, temp1_w, temp5_w);
        UNPCK_SH_SW(temp2_h, temp2_w, temp6_w);
        UNPCK_SH_SW(temp3_h, temp3_w, temp7_w);
        ADD4(temp0_w, vec, temp1_w, vec, temp2_w, vec, temp3_w, vec,
             temp0_w, temp1_w, temp2_w, temp3_w);
        ADD4(temp4_w, vec, temp5_w, vec, temp6_w, vec, temp7_w, vec,
             temp4_w, temp5_w, temp6_w, temp7_w);
        SRARI_W4_SW(temp0_w, temp1_w, temp2_w, temp3_w, 16);
        SRARI_W4_SW(temp4_w, temp5_w, temp6_w, temp7_w, 16);
        PCKEV_H4_SH(temp4_w, temp0_w, temp5_w, temp1_w, temp6_w, temp2_w,
                    temp7_w, temp3_w, temp0_h, temp1_h, temp2_h, temp3_h);
        ADD4(temp0_h, src0_h, temp1_h, src1_h, temp2_h, src2_h, temp3_h,
             src3_h, src0_h, src1_h, src2_h, src3_h);
        CLIP_SH4_0_255(src0_h, src1_h, src2_h, src3_h);
        PCKEV_B2_UB(src1_h, src0_h, src3_h, src2_h, src0, src1);
        ST8x4_UB(src0, src1, dst, dst_stride);
        dst += 4 * dst_stride;
        pred += 4 * pred_stride;
        ptr_temp += 4 * 8;
    }
}

static void mpeg2_idct8x8_msa(WORD16 *src_ptr, WORD32 src_stride,
                              UWORD8 *pred_ptr, WORD32 pred_stride,
                              const WORD16 *idct_coeff_q15,
                              const WORD16 *idct_coeff_q11,
                              UWORD8 *dst, WORD32 dst_stride)
{
    v16u8 pred0, pred1, pred2, pred3, pred4, pred5, pred6, pred7;
    v8i16 zero = {0};
    v8i16 src0, src1, src2, src3, src4, src5, src6, src7, pair0, pair1, pair2;
    v8i16 pair3, odd_h0, odd_h1, odd_h2, odd_h3, pred0_h, pred1_h, pred2_h;
    v8i16 pred3_h, pred4_h, pred5_h, pred6_h, pred7_h;
    v4i32 even_even0_r, even_even1_r, even_even0_l, even_even1_l, even_odd0_r;
    v4i32 even_odd1_r, even_odd0_l, even_odd1_l, even_odd2_r, even_odd3_r;
    v4i32 even_odd2_l, even_odd3_l, even0_r, even1_r, even2_r, even3_r;
    v4i32 even0_l, even1_l, even2_l, even3_l;
    v4i32 odd0_r, odd1_r, odd2_r, odd3_r, odd0_l, odd1_l, odd2_l, odd3_l;

    LD_SH8(src_ptr, src_stride, src0, src1, src2, src3, src4, src5, src6, src7);

    pair0 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[0], idct_coeff_q15[4 * 8]);
    pair1 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[1], idct_coeff_q15[4 * 8 + 1]);
    ILVRL_H2_SH(src4, src0, odd_h0, odd_h1);
    DOTP_SH4_SW(odd_h0, odd_h1, odd_h0, odd_h1, pair0, pair0, pair1, pair1,
                even_even0_r, even_even0_l, even_even1_r, even_even1_l);
    pair0 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[2 * 8], idct_coeff_q15[6 * 8]);
    pair1 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[2 * 8 + 1],
                                 idct_coeff_q15[6 * 8 + 1]);

    ILVRL_H2_SH(src6, src2, odd_h0, odd_h1);
    DOTP_SH4_SW(odd_h0, odd_h1, odd_h0, odd_h1, pair0, pair0, pair1, pair1,
                even_odd0_r, even_odd0_l, even_odd1_r, even_odd1_l);

    BUTTERFLY_8(even_even0_r, even_even1_r, even_even0_l, even_even1_l,
                even_odd1_l, even_odd0_l, even_odd1_r, even_odd0_r, even0_r,
                even1_r, even0_l, even1_l, even2_l, even3_l, even2_r, even3_r);

    pair0 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[1 * 8], idct_coeff_q15[3 * 8]);
    pair1 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[1 * 8 + 1],
                                 idct_coeff_q15[3 * 8 + 1]);
    pair2 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[1 * 8 + 2],
                                 idct_coeff_q15[3 * 8 + 2]);
    pair3 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[1 * 8 + 3],
                                 idct_coeff_q15[3 * 8 + 3]);

    ILVRL_H2_SH(src3, src1, odd_h0, odd_h1);
    DOTP_SH4_SW(odd_h0, odd_h1, odd_h0, odd_h1, pair0, pair0, pair1, pair1,
                odd0_r, odd0_l, odd1_r, odd1_l);
    DOTP_SH4_SW(odd_h0, odd_h1, odd_h0, odd_h1, pair2, pair2, pair3, pair3,
                odd2_r, odd2_l, odd3_r, odd3_l);

    pair0 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[5 * 8], idct_coeff_q15[7 * 8]);
    pair1 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[5 * 8 + 1],
                                 idct_coeff_q15[7 * 8 + 1]);
    pair2 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[5 * 8 + 2],
                                 idct_coeff_q15[7 * 8 + 2]);
    pair3 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[5 * 8 + 3],
                                 idct_coeff_q15[7 * 8 + 3]);
    ILVRL_H2_SH(src7, src5, odd_h2, odd_h3);
    DPADD_SH4_SW(odd_h2, odd_h3, odd_h2, odd_h3, pair0, pair0, pair1, pair1,
                 odd0_r, odd0_l, odd1_r, odd1_l);
    DPADD_SH4_SW(odd_h2, odd_h3, odd_h2, odd_h3, pair2, pair2, pair3, pair3,
                 odd2_r, odd2_l, odd3_r, odd3_l);
    ADD4(even0_r, odd0_r, even1_r, odd1_r, even2_r, odd2_r, even3_r, odd3_r,
         even_odd0_r, even_odd1_r, even_odd2_r, even_odd3_r);
    ADD4(even0_l, odd0_l, even1_l, odd1_l, even2_l, odd2_l, even3_l, odd3_l,
         even_odd0_l, even_odd1_l, even_odd2_l, even_odd3_l);
    SRARI_W4_SW(even_odd0_r, even_odd1_r, even_odd2_r, even_odd3_r, 12);
    SRARI_W4_SW(even_odd0_l, even_odd1_l, even_odd2_l, even_odd3_l, 12);
    PCKEV_H4_SH(even_odd0_l, even_odd0_r, even_odd1_l, even_odd1_r,
                even_odd2_l, even_odd2_r, even_odd3_l, even_odd3_r,
                src0, src1, src2, src3);
    SUB4(even0_r, odd0_r, even1_r, odd1_r, even2_r, odd2_r, even3_r, odd3_r,
         even_odd0_r, even_odd1_r, even_odd2_r, even_odd3_r);
    SUB4(even0_l, odd0_l, even1_l, odd1_l, even2_l, odd2_l, even3_l, odd3_l,
         even_odd0_l, even_odd1_l, even_odd2_l, even_odd3_l);
    SRARI_W4_SW(even_odd0_r, even_odd1_r, even_odd2_r, even_odd3_r, 12);
    SRARI_W4_SW(even_odd0_l, even_odd1_l, even_odd2_l, even_odd3_l, 12);
    PCKEV_H4_SH(even_odd0_l, even_odd0_r, even_odd1_l, even_odd1_r, even_odd2_l,
                even_odd2_r, even_odd3_l, even_odd3_r, src7, src6, src5, src4);
    TRANSPOSE8x8_SH_SH(src0, src1, src2, src3, src4, src5, src6, src7,
                       src0, src1, src2, src3, src4, src5, src6, src7);
    pair0 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[0], idct_coeff_q11[4 * 8]);
    pair1 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[1], idct_coeff_q11[4 * 8 + 1]);
    ILVRL_H2_SH(src4, src0, odd_h0, odd_h1);
    DOTP_SH4_SW(odd_h0, odd_h1, odd_h0, odd_h1, pair0, pair0, pair1, pair1,
                even_even0_r, even_even0_l, even_even1_r, even_even1_l);
    pair0 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[2 * 8], idct_coeff_q11[6 * 8]);
    pair1 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[2 * 8 + 1],
                                 idct_coeff_q11[6 * 8 + 1]);
    ILVRL_H2_SH(src6, src2, odd_h0, odd_h1);
    DOTP_SH4_SW(odd_h0, odd_h1, odd_h0, odd_h1, pair0, pair0, pair1, pair1,
                even_odd0_r, even_odd0_l, even_odd1_r, even_odd1_l);
    BUTTERFLY_8(even_even0_r, even_even1_r, even_even0_l, even_even1_l,
                even_odd1_l, even_odd0_l, even_odd1_r, even_odd0_r, even0_r,
                even1_r, even0_l, even1_l, even2_l, even3_l, even2_r, even3_r);
    pair0 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[1 * 8], idct_coeff_q11[3 * 8]);
    pair1 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[1 * 8 + 1],
                                 idct_coeff_q11[3 * 8 + 1]);
    pair2 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[1 * 8 + 2],
                                 idct_coeff_q11[3 * 8 + 2]);
    pair3 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[1 * 8 + 3],
                                 idct_coeff_q11[3 * 8 + 3]);
    ILVRL_H2_SH(src3, src1, odd_h0, odd_h1);
    DOTP_SH4_SW(odd_h0, odd_h1, odd_h0, odd_h1, pair0, pair0, pair1, pair1,
                odd0_r, odd0_l, odd1_r, odd1_l);
    DOTP_SH4_SW(odd_h0, odd_h1, odd_h0, odd_h1, pair2, pair2, pair3, pair3,
                odd2_r, odd2_l, odd3_r, odd3_l);
    pair0 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[5 * 8], idct_coeff_q11[7 * 8]);
    pair1 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[5 * 8 + 1],
                                 idct_coeff_q11[7 * 8 + 1]);
    pair2 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[5 * 8 + 2],
                                 idct_coeff_q11[7 * 8 + 2]);
    pair3 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[5 * 8 + 3],
                                 idct_coeff_q11[7 * 8 + 3]);
    ILVRL_H2_SH(src7, src5, odd_h2, odd_h3);
    DPADD_SH4_SW(odd_h2, odd_h3, odd_h2, odd_h3, pair0, pair0, pair1, pair1,
                 odd0_r, odd0_l, odd1_r, odd1_l);
    DPADD_SH4_SW(odd_h2, odd_h3, odd_h2, odd_h3, pair2, pair2, pair3, pair3,
                 odd2_r, odd2_l, odd3_r, odd3_l);
    ADD4(even0_r, odd0_r, even1_r, odd1_r, even2_r, odd2_r, even3_r, odd3_r,
         even_odd0_r, even_odd1_r, even_odd2_r, even_odd3_r);
    ADD4(even0_l, odd0_l, even1_l, odd1_l, even2_l, odd2_l, even3_l, odd3_l,
         even_odd0_l, even_odd1_l, even_odd2_l, even_odd3_l);
    SRARI_W4_SW(even_odd0_r, even_odd1_r, even_odd2_r, even_odd3_r, 16);
    SRARI_W4_SW(even_odd0_l, even_odd1_l, even_odd2_l, even_odd3_l, 16);
    PCKEV_H4_SH(even_odd0_l, even_odd0_r, even_odd1_l, even_odd1_r, even_odd2_l,
                even_odd2_r, even_odd3_l, even_odd3_r, src0, src1, src2, src3);
    SUB4(even0_r, odd0_r, even1_r, odd1_r, even2_r, odd2_r, even3_r, odd3_r,
         even_odd0_r, even_odd1_r, even_odd2_r, even_odd3_r);
    SUB4(even0_l, odd0_l, even1_l, odd1_l, even2_l, odd2_l, even3_l, odd3_l,
         even_odd0_l, even_odd1_l, even_odd2_l, even_odd3_l);
    SRARI_W4_SW(even_odd0_r, even_odd1_r, even_odd2_r, even_odd3_r, 16);
    SRARI_W4_SW(even_odd0_l, even_odd1_l, even_odd2_l, even_odd3_l, 16);
    PCKEV_H4_SH(even_odd0_l, even_odd0_r, even_odd1_l, even_odd1_r, even_odd2_l,
                even_odd2_r, even_odd3_l, even_odd3_r, src7, src6, src5, src4);
    TRANSPOSE8x8_SH_SH(src0, src1, src2, src3, src4, src5, src6, src7,
                       src0, src1, src2, src3, src4, src5, src6, src7);
    LD_UB8(pred_ptr, pred_stride, pred0, pred1, pred2, pred3,
           pred4, pred5, pred6, pred7);
    ILVR_B4_SH(zero, pred0, zero, pred1, zero, pred2, zero, pred3,
               pred0_h, pred1_h, pred2_h, pred3_h);
    ILVR_B4_SH(zero, pred4, zero, pred5, zero, pred6, zero, pred7,
               pred4_h, pred5_h, pred6_h, pred7_h);
    ADD4(pred0_h, src0, pred1_h, src1, pred2_h, src2, pred3_h, src3,
         pred0_h, pred1_h, pred2_h, pred3_h);
    ADD4(pred4_h, src4, pred5_h, src5, pred6_h, src6, pred7_h, src7,
         pred4_h, pred5_h, pred6_h, pred7_h);
    CLIP_SH4_0_255(pred0_h, pred1_h, pred2_h, pred3_h);
    CLIP_SH4_0_255(pred4_h, pred5_h, pred6_h, pred7_h);
    PCKEV_B4_UB(pred1_h, pred0_h, pred3_h, pred2_h, pred5_h, pred4_h,
                pred7_h, pred6_h, pred0, pred1, pred2, pred3);
    ST8x4_UB(pred0, pred1, dst, dst_stride);
    ST8x4_UB(pred2, pred3, dst + 4 * dst_stride, dst_stride);
}

static void mpeg2_idct4x4_msa(WORD16 *src_ptr, WORD32 src_stride,
                              UWORD8 *pred_ptr, WORD32 pred_stride,
                              const WORD16 *idct_coeff_q15,
                              const WORD16 *idct_coeff_q11, UWORD8 *dst,
                              WORD32 dst_stride)
{
    v16u8 pred0, pred1, pred2, pred3, pred4, pred5, pred6, pred7;
    v8i16 zero = {0};
    v8i16 src0, src1, src2, src3, src4, src5, src6, src7, odd_h0, odd_h1;
    v8i16 pair0, pair1, pair2, pair3, pred0_h, pred1_h, pred2_h, pred3_h;
    v8i16 pred4_h, pred5_h, pred6_h, pred7_h;
    v4i32 even_even0_r, even_even1_r, even_even0_l, even_even1_l, even_odd0_r;
    v4i32 even_odd1_r, even_odd0_l, even_odd1_l, even_odd2_r, even_odd3_r;
    v4i32 even_odd2_l, even_odd3_l, even0_r, even1_r, even2_r, even3_r;
    v4i32 even0_l, even1_l, even2_l, even3_l;
    v4i32 odd0_r, odd1_r, odd2_r, odd3_r, odd0_l, odd1_l, odd2_l, odd3_l;

    LD_SH4(src_ptr, src_stride, src0, src1, src2, src3);
    pair0 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[0], 0);
    pair1 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[1], 0);
    odd_h0 =  __msa_ilvr_h(zero, src0);
    DOTP_SH2_SW(odd_h0, odd_h0, pair0, pair1, even_even0_r, even_even1_r);

    pair0 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[2 * 8], 0);
    pair1 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[2 * 8 + 1], 0);
    odd_h0 =  __msa_ilvr_h(zero, src2);
    DOTP_SH2_SW(odd_h0, odd_h0, pair0, pair1, even_odd0_r, even_odd1_r);
    BUTTERFLY_4(even_even0_r, even_even1_r, even_odd1_r, even_odd0_r,
                even0_r, even1_r, even2_r, even3_r);
    pair0 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[1 * 8], idct_coeff_q15[3 * 8]);
    pair1 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[1 * 8 + 1],
                                 idct_coeff_q15[3 * 8 + 1]);
    pair2 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[1 * 8 + 2],
                                 idct_coeff_q15[3 * 8 + 2]);
    pair3 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[1 * 8 + 3],
                                 idct_coeff_q15[3 * 8 + 3]);

    odd_h0 =  __msa_ilvr_h(src3, src1);
    DOTP_SH4_SW(odd_h0, odd_h0, odd_h0, odd_h0, pair0, pair1, pair2, pair3,
                odd0_r, odd1_r, odd2_r, odd3_r);
    ADD4(even0_r, odd0_r, even1_r, odd1_r, even2_r, odd2_r, even3_r, odd3_r,
         even_odd0_r, even_odd1_r, even_odd2_r, even_odd3_r);
    SRARI_W4_SW(even_odd0_r, even_odd1_r, even_odd2_r, even_odd3_r, 12);
    PCKEV_H4_SH(zero, even_odd0_r, zero, even_odd1_r, zero, even_odd2_r,
                zero, even_odd3_r, src0, src1, src2, src3);
    SUB4(even0_r, odd0_r, even1_r, odd1_r, even2_r, odd2_r, even3_r, odd3_r,
         even_odd0_r, even_odd1_r, even_odd2_r, even_odd3_r);
    SRARI_W4_SW(even_odd0_r, even_odd1_r, even_odd2_r, even_odd3_r, 12);
    PCKEV_H4_SH(zero, even_odd0_r, zero, even_odd1_r, zero, even_odd2_r,
                zero, even_odd3_r, src7, src6, src5, src4);
    TRANSPOSE4X8_SH_SH(src0, src1, src2, src3, src4, src5, src6, src7,
                       src0, src1, src2, src3, src4, src5, src6, src7);
    pair0 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[0], 0);
    pair1 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[1], 0);
    ILVRL_H2_SH(src4, src0, odd_h0, odd_h1);
    DOTP_SH4_SW(odd_h0, odd_h1, odd_h0, odd_h1, pair0, pair0, pair1, pair1,
                even_even0_r, even_even0_l, even_even1_r, even_even1_l);
    pair0 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[2 * 8], 0);
    pair1 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[2 * 8 + 1], 0);
    ILVRL_H2_SH(src6, src2, odd_h0, odd_h1);
    DOTP_SH4_SW(odd_h0, odd_h1, odd_h0, odd_h1, pair0, pair0, pair1, pair1,
                even_odd0_r, even_odd0_l, even_odd1_r, even_odd1_l);
    BUTTERFLY_8(even_even0_r, even_even1_r, even_even0_l, even_even1_l,
                even_odd1_l, even_odd0_l, even_odd1_r, even_odd0_r, even0_r,
                even1_r, even0_l, even1_l, even2_l, even3_l, even2_r, even3_r);
    pair0 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[1 * 8], idct_coeff_q11[3 * 8]);
    pair1 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[1 * 8 + 1],
                                 idct_coeff_q11[3 * 8 + 1]);
    pair2 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[1 * 8 + 2],
                                 idct_coeff_q11[3 * 8 + 2]);
    pair3 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[1 * 8 + 3],
                                 idct_coeff_q11[3 * 8 + 3]);
    ILVRL_H2_SH(src3, src1, odd_h0, odd_h1);
    DOTP_SH4_SW(odd_h0, odd_h1, odd_h0, odd_h1, pair0, pair0, pair1, pair1,
                odd0_r, odd0_l, odd1_r, odd1_l);
    DOTP_SH4_SW(odd_h0, odd_h1, odd_h0, odd_h1, pair2, pair2, pair3, pair3,
                odd2_r, odd2_l, odd3_r, odd3_l);
    ADD4(even0_r, odd0_r, even1_r, odd1_r, even2_r, odd2_r, even3_r, odd3_r,
         even_odd0_r, even_odd1_r, even_odd2_r, even_odd3_r);
    ADD4(even0_l, odd0_l, even1_l, odd1_l, even2_l, odd2_l, even3_l, odd3_l,
         even_odd0_l, even_odd1_l, even_odd2_l, even_odd3_l);
    SRARI_W4_SW(even_odd0_r, even_odd1_r, even_odd2_r, even_odd3_r, 16);
    SRARI_W4_SW(even_odd0_l, even_odd1_l, even_odd2_l, even_odd3_l, 16);
    PCKEV_H4_SH(even_odd0_l, even_odd0_r, even_odd1_l, even_odd1_r, even_odd2_l,
                even_odd2_r, even_odd3_l, even_odd3_r, src0, src1, src2, src3);
    SUB4(even0_r, odd0_r, even1_r, odd1_r, even2_r, odd2_r, even3_r, odd3_r,
         even_odd0_r, even_odd1_r, even_odd2_r, even_odd3_r);
    SUB4(even0_l, odd0_l, even1_l, odd1_l, even2_l, odd2_l, even3_l, odd3_l,
         even_odd0_l, even_odd1_l, even_odd2_l, even_odd3_l);
    SRARI_W4_SW(even_odd0_r, even_odd1_r, even_odd2_r, even_odd3_r, 16);
    SRARI_W4_SW(even_odd0_l, even_odd1_l, even_odd2_l, even_odd3_l, 16);
    PCKEV_H4_SH(even_odd0_l, even_odd0_r, even_odd1_l, even_odd1_r, even_odd2_l,
                even_odd2_r, even_odd3_l, even_odd3_r, src7, src6, src5, src4);
    TRANSPOSE8x8_SH_SH(src0, src1, src2, src3, src4, src5, src6, src7,
                       src0, src1, src2, src3, src4, src5, src6, src7);
    LD_UB8(pred_ptr, pred_stride, pred0, pred1, pred2, pred3,
           pred4, pred5, pred6, pred7);
    ILVR_B4_SH(zero, pred0, zero, pred1, zero, pred2, zero, pred3,
               pred0_h, pred1_h, pred2_h, pred3_h);
    ILVR_B4_SH(zero, pred4, zero, pred5, zero, pred6, zero, pred7,
               pred4_h, pred5_h, pred6_h, pred7_h);
    ADD4(pred0_h, src0, pred1_h, src1, pred2_h, src2, pred3_h, src3,
         pred0_h, pred1_h, pred2_h, pred3_h);
    ADD4(pred4_h, src4, pred5_h, src5, pred6_h, src6, pred7_h, src7,
         pred4_h, pred5_h, pred6_h, pred7_h);
    CLIP_SH4_0_255(pred0_h, pred1_h, pred2_h, pred3_h);
    CLIP_SH4_0_255(pred4_h, pred5_h, pred6_h, pred7_h);
    PCKEV_B4_UB(pred1_h, pred0_h, pred3_h, pred2_h, pred5_h, pred4_h,
                pred7_h, pred6_h, pred0, pred1, pred2, pred3);
    ST8x4_UB(pred0, pred1, dst, dst_stride);
    ST8x4_UB(pred2, pred3, dst + 4 * dst_stride, dst_stride);
}

static void mpeg2_idct8x4_msa(WORD16 *src_ptr, WORD32 src_stride,
                              UWORD8 *pred_ptr, WORD32 pred_stride,
                              const WORD16 *idct_coeff_q15,
                              const WORD16 *idct_coeff_q11,
                              UWORD8 *dst, WORD32 dst_stride)
{
    v16u8 pred0, pred1, pred2, pred3, pred4, pred5, pred6, pred7;
    v8i16 zero = {0};
    v8i16 src0, src1, src2, src3, src4, src5, src6, src7, pair0, pair1, pair2;
    v8i16 pair3, odd_h0, odd_h1, odd_h2, odd_h3, pred0_h, pred1_h;
    v8i16 pred2_h, pred3_h, pred4_h, pred5_h, pred6_h, pred7_h;
    v4i32 even_even0_r, even_even1_r, even_even0_l, even_even1_l;
    v4i32 even_odd0_r, even_odd1_r, even_odd0_l, even_odd1_l, even_odd2_r;
    v4i32 even_odd3_r, even_odd2_l, even_odd3_l, even0_r, even1_r, even2_r;
    v4i32 even3_r, even0_l, even1_l, even2_l, even3_l;
    v4i32 odd0_r, odd1_r, odd2_r, odd3_r, odd0_l, odd1_l, odd2_l, odd3_l;

    LD_SH4(src_ptr, src_stride, src0, src1, src2, src3);
    pair0 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[0], 0);
    pair1 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[1], 0);
    ILVRL_H2_SH(zero, src0, odd_h0, odd_h1);
    DOTP_SH4_SW(odd_h0, odd_h1, odd_h0, odd_h1, pair0, pair0, pair1, pair1,
                even_even0_r, even_even0_l, even_even1_r, even_even1_l);
    pair0 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[2 * 8], 0);
    pair1 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[2 * 8 + 1], 0);
    ILVRL_H2_SH(zero, src2, odd_h0, odd_h1);
    DOTP_SH4_SW(odd_h0, odd_h1, odd_h0, odd_h1, pair0, pair0, pair1, pair1,
                even_odd0_r, even_odd0_l, even_odd1_r, even_odd1_l);
    BUTTERFLY_8(even_even0_r, even_even1_r, even_even0_l, even_even1_l,
                even_odd1_l, even_odd0_l, even_odd1_r, even_odd0_r, even0_r,
                even1_r, even0_l, even1_l, even2_l, even3_l, even2_r, even3_r);
    pair0 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[1 * 8], idct_coeff_q15[3 * 8]);
    pair1 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[1 * 8 + 1],
                                 idct_coeff_q15[3 * 8 + 1]);
    pair2 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[1 * 8 + 2],
                                 idct_coeff_q15[3 * 8 + 2]);
    pair3 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[1 * 8 + 3],
                                 idct_coeff_q15[3 * 8 + 3]);
    ILVRL_H2_SH(src3, src1, odd_h0, odd_h1);
    DOTP_SH4_SW(odd_h0, odd_h1, odd_h0, odd_h1, pair0, pair0, pair1, pair1,
                odd0_r, odd0_l, odd1_r, odd1_l);
    DOTP_SH4_SW(odd_h0, odd_h1, odd_h0, odd_h1, pair2, pair2, pair3, pair3,
                odd2_r, odd2_l, odd3_r, odd3_l);
    ADD4(even0_r, odd0_r, even1_r, odd1_r, even2_r, odd2_r, even3_r, odd3_r,
         even_odd0_r, even_odd1_r, even_odd2_r, even_odd3_r);
    ADD4(even0_l, odd0_l, even1_l, odd1_l, even2_l, odd2_l, even3_l, odd3_l,
         even_odd0_l, even_odd1_l, even_odd2_l, even_odd3_l);
    SRARI_W4_SW(even_odd0_r, even_odd1_r, even_odd2_r, even_odd3_r, 12);
    SRARI_W4_SW(even_odd0_l, even_odd1_l, even_odd2_l, even_odd3_l, 12);
    PCKEV_H4_SH(even_odd0_l, even_odd0_r, even_odd1_l, even_odd1_r, even_odd2_l,
                even_odd2_r, even_odd3_l, even_odd3_r, src0, src1, src2, src3);
    SUB4(even0_r, odd0_r, even1_r, odd1_r, even2_r, odd2_r, even3_r, odd3_r,
         even_odd0_r, even_odd1_r, even_odd2_r, even_odd3_r);
    SUB4(even0_l, odd0_l, even1_l, odd1_l, even2_l, odd2_l, even3_l, odd3_l,
         even_odd0_l, even_odd1_l, even_odd2_l, even_odd3_l);
    SRARI_W4_SW(even_odd0_r, even_odd1_r, even_odd2_r, even_odd3_r, 12);
    SRARI_W4_SW(even_odd0_l, even_odd1_l, even_odd2_l, even_odd3_l, 12);
    PCKEV_H4_SH(even_odd0_l, even_odd0_r, even_odd1_l, even_odd1_r, even_odd2_l,
                even_odd2_r, even_odd3_l, even_odd3_r, src7, src6, src5, src4);
    TRANSPOSE8x8_SH_SH(src0, src1, src2, src3, src4, src5, src6, src7,
                       src0, src1, src2, src3, src4, src5, src6, src7);
    pair0 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[0], idct_coeff_q11[4 * 8]);
    pair1 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[1], idct_coeff_q11[4 * 8 + 1]);
    ILVRL_H2_SH(src4, src0, odd_h0, odd_h1);
    DOTP_SH4_SW(odd_h0, odd_h1, odd_h0, odd_h1, pair0, pair0, pair1, pair1,
                even_even0_r, even_even0_l, even_even1_r, even_even1_l);
    pair0 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[2 * 8], idct_coeff_q11[6 * 8]);
    pair1 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[2 * 8 + 1],
                                 idct_coeff_q11[6 * 8 + 1]);
    ILVRL_H2_SH(src6, src2, odd_h0, odd_h1);
    DOTP_SH4_SW(odd_h0, odd_h1, odd_h0, odd_h1, pair0, pair0, pair1, pair1,
                even_odd0_r, even_odd0_l, even_odd1_r, even_odd1_l);
    BUTTERFLY_8(even_even0_r, even_even1_r, even_even0_l, even_even1_l,
                even_odd1_l, even_odd0_l, even_odd1_r, even_odd0_r, even0_r,
                even1_r, even0_l, even1_l, even2_l, even3_l, even2_r, even3_r);
    pair0 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[1 * 8], idct_coeff_q11[3 * 8]);
    pair1 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[1 * 8 + 1],
                                 idct_coeff_q11[3 * 8 + 1]);
    pair2 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[1 * 8 + 2],
                                 idct_coeff_q11[3 * 8 + 2]);
    pair3 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[1 * 8 + 3],
                                 idct_coeff_q11[3 * 8 + 3]);
    ILVRL_H2_SH(src3, src1, odd_h0, odd_h1);
    DOTP_SH4_SW(odd_h0, odd_h1, odd_h0, odd_h1, pair0, pair0, pair1, pair1,
                odd0_r, odd0_l, odd1_r, odd1_l);
    DOTP_SH4_SW(odd_h0, odd_h1, odd_h0, odd_h1, pair2, pair2, pair3, pair3,
                odd2_r, odd2_l, odd3_r, odd3_l);
    pair0 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[5 * 8], idct_coeff_q11[7 * 8]);
    pair1 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[5 * 8 + 1],
                                 idct_coeff_q11[7 * 8 + 1]);
    pair2 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[5 * 8 + 2],
                                 idct_coeff_q11[7 * 8 + 2]);
    pair3 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[5 * 8 + 3],
                                 idct_coeff_q11[7 * 8 + 3]);
    ILVRL_H2_SH(src7, src5, odd_h2, odd_h3);
    DPADD_SH4_SW(odd_h2, odd_h3, odd_h2, odd_h3, pair0, pair0, pair1, pair1,
                 odd0_r, odd0_l, odd1_r, odd1_l);
    DPADD_SH4_SW(odd_h2, odd_h3, odd_h2, odd_h3, pair2, pair2, pair3, pair3,
                 odd2_r, odd2_l, odd3_r, odd3_l);
    ADD4(even0_r, odd0_r, even1_r, odd1_r, even2_r, odd2_r, even3_r, odd3_r,
         even_odd0_r, even_odd1_r, even_odd2_r, even_odd3_r);
    ADD4(even0_l, odd0_l, even1_l, odd1_l, even2_l, odd2_l, even3_l, odd3_l,
         even_odd0_l, even_odd1_l, even_odd2_l, even_odd3_l);
    SRARI_W4_SW(even_odd0_r, even_odd1_r, even_odd2_r, even_odd3_r, 16);
    SRARI_W4_SW(even_odd0_l, even_odd1_l, even_odd2_l, even_odd3_l, 16);
    PCKEV_H4_SH(even_odd0_l, even_odd0_r, even_odd1_l, even_odd1_r, even_odd2_l,
                even_odd2_r, even_odd3_l, even_odd3_r, src0, src1, src2, src3);
    SUB4(even0_r, odd0_r, even1_r, odd1_r, even2_r, odd2_r, even3_r, odd3_r,
         even_odd0_r, even_odd1_r, even_odd2_r, even_odd3_r);
    SUB4(even0_l, odd0_l, even1_l, odd1_l, even2_l, odd2_l, even3_l, odd3_l,
         even_odd0_l, even_odd1_l, even_odd2_l, even_odd3_l);
    SRARI_W4_SW(even_odd0_r, even_odd1_r, even_odd2_r, even_odd3_r, 16);
    SRARI_W4_SW(even_odd0_l, even_odd1_l, even_odd2_l, even_odd3_l, 16);
    PCKEV_H4_SH(even_odd0_l, even_odd0_r, even_odd1_l, even_odd1_r, even_odd2_l,
                even_odd2_r, even_odd3_l, even_odd3_r, src7, src6, src5, src4);
    TRANSPOSE8x8_SH_SH(src0, src1, src2, src3, src4, src5, src6, src7,
                       src0, src1, src2, src3, src4, src5, src6, src7);
    LD_UB8(pred_ptr, pred_stride, pred0, pred1, pred2, pred3,
           pred4, pred5, pred6, pred7);
    ILVR_B4_SH(zero, pred0, zero, pred1, zero, pred2, zero, pred3,
               pred0_h, pred1_h, pred2_h, pred3_h);
    ILVR_B4_SH(zero, pred4, zero, pred5, zero, pred6, zero, pred7,
               pred4_h, pred5_h, pred6_h, pred7_h);
    ADD4(pred0_h, src0, pred1_h, src1, pred2_h, src2, pred3_h, src3,
         pred0_h, pred1_h, pred2_h, pred3_h);
    ADD4(pred4_h, src4, pred5_h, src5, pred6_h, src6, pred7_h, src7,
         pred4_h, pred5_h, pred6_h, pred7_h);
    CLIP_SH4_0_255(pred0_h, pred1_h, pred2_h, pred3_h);
    CLIP_SH4_0_255(pred4_h, pred5_h, pred6_h, pred7_h);
    PCKEV_B4_UB(pred1_h, pred0_h, pred3_h, pred2_h, pred5_h, pred4_h,
                pred7_h, pred6_h, pred0, pred1, pred2, pred3);
    ST8x4_UB(pred0, pred1, dst, dst_stride);
    ST8x4_UB(pred2, pred3, dst + 4 * dst_stride, dst_stride);
}

static void mpeg2_idct4x8_msa(WORD16 *src_ptr, WORD32 src_stride,
                              UWORD8 *pred_ptr, WORD32 pred_stride,
                              const WORD16 *idct_coeff_q15,
                              const WORD16 *idct_coeff_q11,
                              UWORD8 *dst, WORD32 dst_stride)
{
    v16u8 pred0, pred1, pred2, pred3, pred4, pred5, pred6, pred7;
    v8i16 zero = {0};
    v8i16 src0, src1, src2, src3, src4, src5, src6, src7, pair0, pair1, pair2;
    v8i16 pair3, odd_h0, odd_h1, odd_h2, pred0_h, pred1_h, pred2_h;
    v8i16 pred3_h, pred4_h, pred5_h, pred6_h, pred7_h;
    v4i32 even_even0_r, even_even1_r, even_even0_l, even_even1_l, even_odd0_r;
    v4i32 even_odd1_r, even_odd0_l, even_odd1_l, even_odd2_r, even_odd3_r;
    v4i32 even_odd2_l, even_odd3_l, even0_r, even1_r, even2_r, even3_r;
    v4i32 even0_l, even1_l, even2_l, even3_l;
    v4i32 odd0_r, odd1_r, odd2_r, odd3_r, odd0_l, odd1_l, odd2_l, odd3_l;

    LD_SH8(src_ptr, src_stride, src0, src1, src2, src3, src4, src5, src6, src7);
    pair0 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[0], idct_coeff_q15[4 * 8]);
    pair1 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[1], idct_coeff_q15[4 * 8 + 1]);
    odd_h0 =  __msa_ilvr_h(src4, src0);
    DOTP_SH2_SW(odd_h0, odd_h0, pair0, pair1, even_even0_r, even_even1_r);
    pair0 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[2 * 8], idct_coeff_q15[6 * 8]);
    pair1 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[2 * 8 + 1],
                                 idct_coeff_q15[6 * 8 + 1]);
    odd_h0 =  __msa_ilvr_h(src6, src2);
    DOTP_SH2_SW(odd_h0, odd_h0, pair0, pair1, even_odd0_r, even_odd1_r);
    BUTTERFLY_4(even_even0_r, even_even1_r, even_odd1_r, even_odd0_r,
                even0_r, even1_r, even2_r, even3_r);
    pair0 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[1 * 8], idct_coeff_q15[3 * 8]);
    pair1 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[1 * 8 + 1],
                                 idct_coeff_q15[3 * 8 + 1]);
    pair2 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[1 * 8 + 2],
                                 idct_coeff_q15[3 * 8 + 2]);
    pair3 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[1 * 8 + 3],
                                 idct_coeff_q15[3 * 8 + 3]);

    odd_h0 =  __msa_ilvr_h(src3, src1);
    DOTP_SH4_SW(odd_h0, odd_h0, odd_h0, odd_h0, pair0, pair1, pair2, pair3,
                odd0_r, odd1_r, odd2_r, odd3_r);

    pair0 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[5 * 8], idct_coeff_q15[7 * 8]);
    pair1 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[5 * 8 + 1],
                                 idct_coeff_q15[7 * 8 + 1]);
    pair2 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[5 * 8 + 2],
                                 idct_coeff_q15[7 * 8 + 2]);
    pair3 = MPEG2_SET_COSPI_PAIR(idct_coeff_q15[5 * 8 + 3],
                                 idct_coeff_q15[7 * 8 + 3]);
    odd_h2 =  __msa_ilvr_h(src7, src5);
    DPADD_SH4_SW(odd_h2, odd_h2, odd_h2, odd_h2, pair0, pair1, pair2, pair3,
                 odd0_r, odd1_r, odd2_r, odd3_r);
    ADD4(even0_r, odd0_r, even1_r, odd1_r, even2_r, odd2_r, even3_r, odd3_r,
         even_odd0_r, even_odd1_r, even_odd2_r, even_odd3_r);
    SRARI_W4_SW(even_odd0_r, even_odd1_r, even_odd2_r, even_odd3_r, 12);
    PCKEV_H4_SH(zero, even_odd0_r, zero, even_odd1_r, zero, even_odd2_r,
                zero, even_odd3_r, src0, src1, src2, src3);
    SUB4(even0_r, odd0_r, even1_r, odd1_r, even2_r, odd2_r, even3_r, odd3_r,
         even_odd0_r, even_odd1_r, even_odd2_r, even_odd3_r);
    SRARI_W4_SW(even_odd0_r, even_odd1_r, even_odd2_r, even_odd3_r, 12);
    PCKEV_H4_SH(zero, even_odd0_r, zero, even_odd1_r, zero, even_odd2_r,
                zero, even_odd3_r, src7, src6, src5, src4);
    TRANSPOSE4X8_SH_SH(src0, src1, src2, src3, src4, src5, src6, src7,
                       src0, src1, src2, src3, src4, src5, src6, src7);
    pair0 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[0], 0);
    pair1 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[1], 0);
    ILVRL_H2_SH(src4, src0, odd_h0, odd_h1);
    DOTP_SH4_SW(odd_h0, odd_h1, odd_h0, odd_h1, pair0, pair0, pair1, pair1,
                even_even0_r, even_even0_l, even_even1_r, even_even1_l);
    pair0 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[2 * 8], 0);
    pair1 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[2 * 8 + 1], 0);
    ILVRL_H2_SH(src6, src2, odd_h0, odd_h1);
    DOTP_SH4_SW(odd_h0, odd_h1, odd_h0, odd_h1, pair0, pair0, pair1, pair1,
                even_odd0_r, even_odd0_l, even_odd1_r, even_odd1_l);
    BUTTERFLY_8(even_even0_r, even_even1_r, even_even0_l, even_even1_l,
                even_odd1_l, even_odd0_l, even_odd1_r, even_odd0_r, even0_r,
                even1_r, even0_l, even1_l, even2_l, even3_l, even2_r, even3_r);

    pair0 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[1 * 8], idct_coeff_q11[3 * 8]);
    pair1 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[1 * 8 + 1],
                                 idct_coeff_q11[3 * 8 + 1]);
    pair2 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[1 * 8 + 2],
                                 idct_coeff_q11[3 * 8 + 2]);
    pair3 = MPEG2_SET_COSPI_PAIR(idct_coeff_q11[1 * 8 + 3],
                                 idct_coeff_q11[3 * 8 + 3]);
    ILVRL_H2_SH(src3, src1, odd_h0, odd_h1);
    DOTP_SH4_SW(odd_h0, odd_h1, odd_h0, odd_h1, pair0, pair0, pair1, pair1,
                odd0_r, odd0_l, odd1_r, odd1_l);
    DOTP_SH4_SW(odd_h0, odd_h1, odd_h0, odd_h1, pair2, pair2, pair3, pair3,
                odd2_r, odd2_l, odd3_r, odd3_l);
    ADD4(even0_r, odd0_r, even1_r, odd1_r, even2_r, odd2_r, even3_r, odd3_r,
         even_odd0_r, even_odd1_r, even_odd2_r, even_odd3_r);
    ADD4(even0_l, odd0_l, even1_l, odd1_l, even2_l, odd2_l, even3_l, odd3_l,
         even_odd0_l, even_odd1_l, even_odd2_l, even_odd3_l);
    SRARI_W4_SW(even_odd0_r, even_odd1_r, even_odd2_r, even_odd3_r, 16);
    SRARI_W4_SW(even_odd0_l, even_odd1_l, even_odd2_l, even_odd3_l, 16);
    PCKEV_H4_SH(even_odd0_l, even_odd0_r, even_odd1_l, even_odd1_r, even_odd2_l,
                even_odd2_r, even_odd3_l, even_odd3_r, src0, src1, src2, src3);
    SUB4(even0_r, odd0_r, even1_r, odd1_r, even2_r, odd2_r, even3_r, odd3_r,
         even_odd0_r, even_odd1_r, even_odd2_r, even_odd3_r);
    SUB4(even0_l, odd0_l, even1_l, odd1_l, even2_l, odd2_l, even3_l, odd3_l,
         even_odd0_l, even_odd1_l, even_odd2_l, even_odd3_l);
    SRARI_W4_SW(even_odd0_r, even_odd1_r, even_odd2_r, even_odd3_r, 16);
    SRARI_W4_SW(even_odd0_l, even_odd1_l, even_odd2_l, even_odd3_l, 16);
    PCKEV_H4_SH(even_odd0_l, even_odd0_r, even_odd1_l, even_odd1_r, even_odd2_l,
                even_odd2_r, even_odd3_l, even_odd3_r, src7, src6, src5, src4);
    TRANSPOSE8x8_SH_SH(src0, src1, src2, src3, src4, src5, src6, src7,
                       src0, src1, src2, src3, src4, src5, src6, src7);
    LD_UB8(pred_ptr, pred_stride, pred0, pred1, pred2, pred3,
           pred4, pred5, pred6, pred7);
    ILVR_B4_SH(zero, pred0, zero, pred1, zero, pred2, zero, pred3,
               pred0_h, pred1_h, pred2_h, pred3_h);
    ILVR_B4_SH(zero, pred4, zero, pred5, zero, pred6, zero, pred7,
               pred4_h, pred5_h, pred6_h, pred7_h);
    ADD4(pred0_h, src0, pred1_h, src1, pred2_h, src2, pred3_h, src3,
         pred0_h, pred1_h, pred2_h, pred3_h);
    ADD4(pred4_h, src4, pred5_h, src5, pred6_h, src6, pred7_h, src7,
         pred4_h, pred5_h, pred6_h, pred7_h);
    CLIP_SH4_0_255(pred0_h, pred1_h, pred2_h, pred3_h);
    CLIP_SH4_0_255(pred4_h, pred5_h, pred6_h, pred7_h);
    PCKEV_B4_UB(pred1_h, pred0_h, pred3_h, pred2_h, pred5_h, pred4_h,
                pred7_h, pred6_h, pred0, pred1, pred2, pred3);
    ST8x4_UB(pred0, pred1, dst, dst_stride);
    ST8x4_UB(pred2, pred3, dst + 4 * dst_stride, dst_stride);
}

static void add_const_clip_8width_msa(WORD16 src, UWORD8 *pred,
                                      WORD32 pred_stride,
                                      UWORD8 *dst, WORD32 dst_stride)
{
    v16u8 src0, src1, src2, src3, src4, src5, src6, src7;
    v8i16 src0_h, src1_h, src2_h, src3_h, src4_h, src5_h, src6_h, src7_h;
    v8i16 vec, out0, out1, out2, out3, out4, out5, out6, out7;
    v16i8 zero = { 0 };

    vec = __msa_fill_h(src);

    LD_UB8(pred, pred_stride, src0, src1, src2, src3, src4, src5, src6, src7);

    ILVR_B4_SH(zero, src0, zero, src1, zero, src2, zero, src3,
               src0_h, src1_h, src2_h, src3_h);
    ILVR_B4_SH(zero, src4, zero, src5, zero, src6, zero, src7,
               src4_h, src5_h, src6_h, src7_h);
    ADD4(src0_h, vec, src1_h, vec, src2_h, vec, src3_h, vec,
         out0, out1, out2, out3);
    ADD4(src4_h, vec, src5_h, vec, src6_h, vec, src7_h, vec,
         out4, out5, out6, out7);
    CLIP_SH4_0_255(out0, out1, out2, out3);
    CLIP_SH4_0_255(out4, out5, out6, out7);
    PCKEV_B4_SH(out1, out0, out3, out2, out5, out4, out7, out6,
                out0, out1, out2, out3)

    ST8x4_UB(out0, out1, dst, dst_stride);
    ST8x4_UB(out2, out3, dst + 4 * dst_stride, dst_stride);
    pred += 8 * pred_stride;
    dst += 8 * dst_stride;
}

void impeg2_idct_recon_dc_msa(WORD16 *src, WORD16 *tmp, UWORD8 *pred,
                              UWORD8 *dst, WORD32 src_stride,
                              WORD32 pred_stride, WORD32 dst_stride,
                              WORD32 zero_cols, WORD32 zero_rows)
{
    WORD32 val;
    UNUSED(tmp);
    UNUSED(src_stride);
    UNUSED(zero_cols);
    UNUSED(zero_rows);

    val = src[0] * gai2_impeg2_idct_q15[0];
    val = ((val + IDCT_STG1_ROUND) >> IDCT_STG1_SHIFT);
    val = val * gai2_impeg2_idct_q11[0];
    val = ((val + IDCT_STG2_ROUND) >> IDCT_STG2_SHIFT);

    add_const_clip_8width_msa(val, pred, pred_stride, dst, dst_stride);
}

void impeg2_idct_recon_dc_mismatch_msa(WORD16 *src, WORD16 *tmp, UWORD8 *pred,
                                       UWORD8 *dst, WORD32 src_stride,
                                       WORD32 pred_stride, WORD32 dst_stride,
                                       WORD32 zero_cols, WORD32 zero_rows)
{
    UNUSED(tmp);
    UNUSED(src_stride);
    UNUSED(zero_cols);
    UNUSED(zero_rows);

    mpeg2_idct_recon_dc_add_msa(src[0], pred, pred_stride,
                                gai2_impeg2_idct_q15, gai2_impeg2_idct_q11,
                                gai2_impeg2_mismatch_stg2_additive,
                                dst, dst_stride);
}

void impeg2_idct_recon_msa(WORD16 *src, WORD16 *tmp, UWORD8 *pred, UWORD8 *dst,
                           WORD32 src_stride, WORD32 pred_stride,
                           WORD32 dst_stride, WORD32 zero_cols,
                           WORD32 zero_rows)
{
    UNUSED(tmp);

    if(0xF0 == (zero_rows & 0xF0))
    {
        if(0xF0 == (zero_cols & 0xF0))
        {
            mpeg2_idct4x4_msa(src, src_stride, pred, pred_stride,
                              gai2_impeg2_idct_q15, gai2_impeg2_idct_q11,
                              dst, dst_stride);
        }
        else
        {
            mpeg2_idct8x4_msa(src, src_stride, pred, pred_stride,
                              gai2_impeg2_idct_q15, gai2_impeg2_idct_q11,
                              dst, dst_stride);
        }
    }
    else
    {
        if(0xF0 == (zero_cols & 0xF0))
        {
            mpeg2_idct4x8_msa(src, src_stride, pred, pred_stride,
                              gai2_impeg2_idct_q15, gai2_impeg2_idct_q11,
                              dst, dst_stride);
        }
        else
        {
            mpeg2_idct8x8_msa(src, src_stride, pred, pred_stride,
                              gai2_impeg2_idct_q15, gai2_impeg2_idct_q11,
                              dst, dst_stride);
        }
    }
}
