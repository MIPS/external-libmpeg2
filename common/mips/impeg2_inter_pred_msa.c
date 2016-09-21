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
#include "impeg2_inter_pred.h"
#include "impeg2_macros_msa.h"

static void m4v_h263_horiz_filter_8width_msa(UWORD8 *src, WORD32 src_stride,
                                             UWORD8 *dst, WORD32 dst_stride,
                                             WORD32 height)
{
    WORD32 ht_cnt;
    v16u8 inp0, inp1, inp2, inp3, inp4, inp5, inp6, inp7, inp0_sld1, inp1_sld1;
    v16u8 inp2_sld1, inp3_sld1, inp4_sld1, inp5_sld1, inp6_sld1, inp7_sld1;

    for(ht_cnt = height >> 3; ht_cnt--;)
    {
        LD_UB8(src, src_stride, inp0, inp1, inp2, inp3, inp4, inp5, inp6, inp7);
        SLDI_B4_0_UB(inp0, inp1, inp2, inp3,
                     inp0_sld1, inp1_sld1, inp2_sld1, inp3_sld1, 1);
        SLDI_B4_0_UB(inp4, inp5, inp6, inp7,
                     inp4_sld1, inp5_sld1, inp6_sld1, inp7_sld1, 1);
        AVER_ST8x4_UB(inp0_sld1, inp0, inp1_sld1, inp1, inp2_sld1, inp2,
                      inp3_sld1, inp3, dst, dst_stride);
        AVER_ST8x4_UB(inp4_sld1, inp4, inp5_sld1, inp5, inp6_sld1, inp6,
                      inp7_sld1, inp7, dst + 4 * dst_stride, dst_stride);

        src += (8 * src_stride);
        dst += (8 * dst_stride);
    }
}

static void m4v_h263_vert_filter_8width_msa(UWORD8 *src, WORD32 src_stride,
                                            UWORD8 *dst, WORD32 dst_stride,
                                            WORD32 height)
{
    WORD32 ht_cnt;
    v16u8 inp0, inp1, inp2, inp3, inp4, inp5, inp6, inp7, inp8;

    inp0 = LD_UB(src);
    src += src_stride;

    for(ht_cnt = height >> 3; ht_cnt--;)
    {
        LD_UB8(src, src_stride, inp1, inp2, inp3, inp4, inp5, inp6, inp7, inp8);
        src += (8 * src_stride);

        AVER_ST8x4_UB(inp1, inp0, inp2, inp1, inp3, inp2, inp4, inp3,
                      dst, dst_stride);
        AVER_ST8x4_UB(inp5, inp4, inp6, inp5, inp7, inp6, inp8, inp7,
                      dst + 4 * dst_stride, dst_stride);

        inp0 = inp8;
        dst += (8 * dst_stride);
    }
}

static void m4v_h263_hv_filter_8width_msa(UWORD8 *src, WORD32 src_stride,
                                          UWORD8 * dst, WORD32 dst_stride,
                                          WORD32 height)
{
    WORD32 ht_cnt;
    v16u8 inp0, inp1, inp2, inp3, inp4, inp5, inp6, inp7, inp8, inp0_sld2;
    v16u8 inp1_sld2, inp2_sld2, inp3_sld2, inp4_sld2, inp5_sld2, inp6_sld2;
    v16u8 inp7_sld2, inp8_sld2;
    v16i8 tmp0, tmp1, zero = { 0 };
    v8u16 src0, src1, src2, src3, src4, src5, src6, src7, src8, src0_sld2;
    v8u16 src1_sld2, src2_sld2, src3_sld2, src4_sld2, src5_sld2, src6_sld2;
    v8u16 src7_sld2, src8_sld2, sum0, sum1, sum2, sum3, sum4, sum5, sum6, sum7;

    for(ht_cnt = height >> 3; ht_cnt--;)
    {
        LD_UB8(src, src_stride, inp0, inp1, inp2, inp3, inp4, inp5, inp6, inp7);
        src += (8 * src_stride);
        inp8 = LD_UB(src);
        SLDI_B4_0_UB(inp0, inp1, inp2, inp3, inp0_sld2, inp1_sld2, inp2_sld2,
                     inp3_sld2, 1);
        SLDI_B4_0_UB(inp4, inp5, inp6, inp7, inp4_sld2, inp5_sld2, inp6_sld2,
                     inp7_sld2, 1);
        inp8_sld2 = (v16u8)__msa_sldi_b(zero, (v16i8)inp8, 1);
        ILVR_B8_UH(inp0_sld2, inp0, inp1_sld2, inp1, inp2_sld2, inp2,
                   inp3_sld2, inp3, inp4_sld2, inp4, inp5_sld2, inp5,
                   inp6_sld2, inp6, inp7_sld2, inp7, src0, src1, src2,
                   src3, src4, src5, src6, src7);
        src8 = (v8u16)__msa_ilvr_b((v16i8)inp8_sld2, (v16i8)inp8);
        HADD_UB4_UH(src0, src1, src2, src3, src0_sld2, src1_sld2, src2_sld2,
                    src3_sld2);
        HADD_UB4_UH(src4, src5, src6, src7, src4_sld2, src5_sld2, src6_sld2,
                    src7_sld2);
        src8_sld2 = __msa_hadd_u_h((v16u8)src8, (v16u8)src8);
        sum0 = src0_sld2 + src1_sld2;
        sum1 = src1_sld2 + src2_sld2;
        sum2 = src2_sld2 + src3_sld2;
        sum3 = src3_sld2 + src4_sld2;
        sum4 = src4_sld2 + src5_sld2;
        sum5 = src5_sld2 + src6_sld2;
        sum6 = src6_sld2 + src7_sld2;
        sum7 = src7_sld2 + src8_sld2;
        SRARI_H4_UH(sum0, sum1, sum2, sum3, 2);
        SRARI_H4_UH(sum4, sum5, sum6, sum7, 2);

        PCKEV_B2_SB(sum1, sum0, sum3, sum2, tmp0, tmp1);
        ST8x4_UB(tmp0, tmp1, dst, dst_stride);
        dst += 4 * dst_stride;
        PCKEV_B2_SB(sum5, sum4, sum7, sum6, tmp0, tmp1);
        ST8x4_UB(tmp0, tmp1, dst, dst_stride);
        dst += 4 * dst_stride;
    }
}

static void copy_8x8_msa(UWORD8 *src, WORD32 src_stride,
                         UWORD8 *dst, WORD32 dst_stride)
{
  uint64_t src0, src1, src2, src3;

  LD4(src, src_stride, src0, src1, src2, src3);
  src += (4 * src_stride);
  SD4(src0, src1, src2, src3, dst, dst_stride);
  dst += (4 * dst_stride);

  LD4(src, src_stride, src0, src1, src2, src3);
  SD4(src0, src1, src2, src3, dst, dst_stride);
}

static void copy_16x16_msa(UWORD8 *src, WORD32 src_stride,
                           UWORD8 *dst, WORD32 dst_stride)
{
    v16u8 src0, src1, src2, src3, src4, src5, src6, src7;
    v16u8 src8, src9, src10, src11, src12, src13, src14, src15;

    LD_UB8(src, src_stride, src0, src1, src2, src3, src4, src5, src6, src7);
    src += (8 * src_stride);
    LD_UB8(src, src_stride,
           src8, src9, src10, src11, src12, src13, src14, src15);

    ST_UB8(src0, src1, src2, src3, src4, src5, src6, src7, dst, dst_stride);
    dst += (8 * dst_stride);
    ST_UB8(src8, src9, src10, src11, src12, src13, src14, src15,
           dst, dst_stride);
}

static void avg_src_width8_msa(UWORD8 *src1_ptr, WORD32 src1_stride,
                               UWORD8 *src2_ptr, WORD32 src2_stride,
                               UWORD8 *dst, WORD32 dst_stride,
                               WORD32 height)
{
    WORD32 cnt;
    uint64_t src0_d, src1_d, src2_d, src3_d;
    v16u8 src0 = { 0 };
    v16u8 src1 = { 0 };
    v16u8 src2 = { 0 };
    v16u8 src3 = { 0 };
    v16u8 res0, res1;

    for(cnt = (height >> 2); cnt--;)
    {
        LD4(src1_ptr, src1_stride, src0_d, src1_d, src2_d, src3_d);
        src1_ptr += (4 * src1_stride);
        INSERT_D2_UB(src0_d, src1_d, src0);
        INSERT_D2_UB(src2_d, src3_d, src1);

        LD4(src2_ptr, src2_stride, src0_d, src1_d, src2_d, src3_d);
        src2_ptr += (4 * src2_stride);
        INSERT_D2_UB(src0_d, src1_d, src2);
        INSERT_D2_UB(src2_d, src3_d, src3);

        AVER_UB2_UB(src0, src2, src1, src3, res0, res1);
        ST8x4_UB(res0, res1, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

static void avg_src_width16_msa(UWORD8 *src1_ptr, WORD32 src1_stride,
                                UWORD8 *src2_ptr, WORD32 src2_stride,
                                UWORD8 *dst, WORD32 dst_stride,
                                WORD32 height)
{
    WORD32 cnt;
    v16u8 src0, src1, src2, src3, src4, src5, src6, src7;
    v16u8 src8, src9, src10, src11, src12, src13, src14, src15;

    for(cnt = (height >> 3); cnt--;)
    {
        LD_UB8(src1_ptr, src1_stride,
               src0, src1, src2, src3, src4, src5, src6, src7);
        src1_ptr += (8 * src1_stride);
        LD_UB8(src2_ptr, src2_stride,
               src8, src9, src10, src11, src12, src13, src14, src15);
        src2_ptr += (8 * src2_stride);

        AVER_ST16x4_UB(src0, src8, src1, src9, src2, src10, src3, src11,
                       dst, dst_stride);
        dst += (4 * dst_stride);
        AVER_ST16x4_UB(src4, src12, src5, src13, src6, src14, src7, src15,
                       dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

void impeg2_mc_halfx_halfy_8x8_msa(UWORD8 *dst, UWORD8 *ref,
                                   UWORD32 ref_stride, UWORD32 dst_stride)
{
    m4v_h263_hv_filter_8width_msa(ref, ref_stride, dst, dst_stride, 8);
}

void impeg2_mc_halfx_fully_8x8_msa(UWORD8 *dst, UWORD8 *ref,
                                   UWORD32 ref_stride, UWORD32 dst_stride)
{
    m4v_h263_horiz_filter_8width_msa(ref, ref_stride, dst, dst_stride, 8);
}

void impeg2_mc_fullx_halfy_8x8_msa(UWORD8 *dst, UWORD8 *ref,
                                   UWORD32 ref_stride, UWORD32 dst_stride)
{
    m4v_h263_vert_filter_8width_msa(ref, ref_stride, dst, dst_stride, 8);
}

void impeg2_mc_fullx_fully_8x8_msa(UWORD8 *dst, UWORD8 *ref,
                                   UWORD32 ref_stride, UWORD32 dst_stride)
{
    copy_8x8_msa(ref, ref_stride, dst, dst_stride);
}

void impeg2_copy_mb_msa(yuv_buf_t *src_buf, yuv_buf_t *dst_buf,
                        UWORD32 src_stride, UWORD32 dst_stride)
{
    copy_16x16_msa(src_buf->pu1_y, src_stride, dst_buf->pu1_y, dst_stride);

    src_stride >>= 1;
    dst_stride >>= 1;

    copy_8x8_msa(src_buf->pu1_u, src_stride, dst_buf->pu1_u, dst_stride);

    copy_8x8_msa(src_buf->pu1_v, src_stride, dst_buf->pu1_v, dst_stride);
}

void impeg2_interpolate_msa(yuv_buf_t *src1_buf, yuv_buf_t *src2_buf,
                            yuv_buf_t *dst_buf, UWORD32 dst_stride)
{
    UWORD8 *src1, *src2, *dst;

    src1 = src1_buf->pu1_y;
    src2 = src2_buf->pu1_y;
    dst = dst_buf->pu1_y;
    avg_src_width16_msa(src1, 16, src2, 16, dst, dst_stride, 16);

    dst_stride >>= 1;

    src1 = src1_buf->pu1_u;
    src2 = src2_buf->pu1_u;
    dst = dst_buf->pu1_u;
    avg_src_width8_msa(src1, 8, src2, 8, dst, dst_stride, 8);

    src1 = src1_buf->pu1_v;
    src2 = src2_buf->pu1_v;
    dst = dst_buf->pu1_v;
    avg_src_width8_msa(src1, 8, src2, 8, dst, dst_stride, 8);
}
