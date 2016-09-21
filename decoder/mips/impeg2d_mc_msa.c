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
#include <string.h>

#include "iv_datatypedef.h"
#include "impeg2_buf_mgr.h"
#include "impeg2_disp_mgr.h"
#include "impeg2_defs.h"
#include "impeg2_platform_macros.h"
#include "impeg2_inter_pred.h"
#include "impeg2_idct.h"
#include "impeg2_mem_func.h"
#include "impeg2_format_conv.h"
#include "impeg2_macros.h"
#include "impeg2d.h"
#include "impeg2d_bitstream.h"
#include "impeg2d_structs.h"
#include "impeg2d_mc.h"
#include "impeg2_macros_msa.h"

static void m4v_h263_horiz_filter_16width_msa(UWORD8 *src, WORD32 src_stride,
                                              UWORD8 *dst, WORD32 dst_stride,
                                              WORD32 height)
{
    WORD32 ht_cnt;
    v16u8 inp0, inp1, inp2, inp3, inp4, inp5, inp6, inp7, inp0_sld1, inp1_sld1;
    v16u8 inp2_sld1, inp3_sld1, inp4_sld1, inp5_sld1, inp6_sld1, inp7_sld1;

    for(ht_cnt = height >> 3; ht_cnt--;)
    {
        LD_UB8(src, src_stride, inp0, inp1, inp2, inp3, inp4, inp5, inp6, inp7);
        inp0_sld1 = (v16u8)__msa_fill_b(src[16]);
        inp1_sld1 = (v16u8)__msa_fill_b(src[16 + src_stride]);
        inp2_sld1 = (v16u8)__msa_fill_b(src[16 + 2 * src_stride]);
        inp3_sld1 = (v16u8)__msa_fill_b(src[16 + 3 * src_stride]);
        inp4_sld1 = (v16u8)__msa_fill_b(src[16 + 4 * src_stride]);
        inp5_sld1 = (v16u8)__msa_fill_b(src[16 + 5 * src_stride]);
        inp6_sld1 = (v16u8)__msa_fill_b(src[16 + 6 * src_stride]);
        inp7_sld1 = (v16u8)__msa_fill_b(src[16 + 7 * src_stride]);
        src += (8 * src_stride);

        SLDI_B2_UB(inp0_sld1, inp1_sld1, inp0, inp1, inp0_sld1, inp1_sld1, 1);
        SLDI_B2_UB(inp2_sld1, inp3_sld1, inp2, inp3, inp2_sld1, inp3_sld1, 1);
        SLDI_B2_UB(inp4_sld1, inp5_sld1, inp4, inp5, inp4_sld1, inp5_sld1, 1);
        SLDI_B2_UB(inp6_sld1, inp7_sld1, inp6, inp7, inp6_sld1, inp7_sld1, 1);

        AVER_ST16x4_UB(inp0_sld1, inp0, inp1_sld1, inp1, inp2_sld1, inp2,
                       inp3_sld1, inp3, dst, dst_stride);
        dst += (4 * dst_stride);
        AVER_ST16x4_UB(inp4_sld1, inp4, inp5_sld1, inp5, inp6_sld1, inp6,
                       inp7_sld1, inp7, dst, dst_stride);
        dst += (4 * dst_stride);
    }
}

static void m4v_h263_vert_filter_16width_msa(UWORD8 *src, WORD32 src_stride,
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
        src += 8 * src_stride;
        AVER_ST16x4_UB(inp1, inp0, inp2, inp1, inp3, inp2, inp4, inp3,
                       dst, dst_stride);
        dst += (4 * dst_stride);
        AVER_ST16x4_UB(inp5, inp4, inp6, inp5, inp7, inp6, inp8, inp7,
                       dst, dst_stride);
        dst += (4 * dst_stride);

        inp0 = inp8;
    }
}

static void m4v_h263_hv_filter_16width_msa(UWORD8 *src, WORD32 src_stride,
                                           UWORD8 * dst, WORD32 dst_stride,
                                           WORD32 height)
{
    WORD32 ht_cnt;
    v16u8 inp0, inp1, inp2, inp3, inp4, inp5, inp6, inp7, inp8;
    v16u8 inp0_sld2, inp1_sld2, inp2_sld2, inp3_sld2, inp4_sld2;
    v16u8 inp5_sld2, inp6_sld2, inp7_sld2, inp8_sld2;
    v16i8 tmp0, tmp1, tmp2, tmp3;
    v8u16 src0, src1, src2, src3, src4, src5, src6, src7, src8, src9, src10;
    v8u16 src11, src12, src13, src14, src15, src16, src17, src0_sld2;
    v8u16 src1_sld2, src2_sld2, src3_sld2, src4_sld2, src5_sld2, src6_sld2;
    v8u16 src7_sld2, src8_sld2, src9_sld2, src10_sld2, src11_sld2, src12_sld2;
    v8u16 src13_sld2, src14_sld2, src15_sld2, src16_sld2, src17_sld2;
    v8u16 sum0, sum1, sum2, sum3, sum4, sum5, sum6, sum7;
    v8u16 sum8, sum9, sum10, sum11, sum12, sum13, sum14, sum15;

    for(ht_cnt = height >> 3; ht_cnt--;)
    {
        LD_UB8(src, src_stride, inp0, inp1, inp2, inp3, inp4, inp5, inp6, inp7);
        inp8 = LD_UB(src + 8 * src_stride);
        inp0_sld2 = (v16u8)__msa_fill_b(src[16]);
        inp1_sld2 = (v16u8)__msa_fill_b(src[16 + src_stride]);
        inp2_sld2 = (v16u8)__msa_fill_b(src[16 + 2 *src_stride]);
        inp3_sld2 = (v16u8)__msa_fill_b(src[16 + 3 *src_stride]);
        inp4_sld2 = (v16u8)__msa_fill_b(src[16 + 4 *src_stride]);
        inp5_sld2 = (v16u8)__msa_fill_b(src[16 + 5 *src_stride]);
        inp6_sld2 = (v16u8)__msa_fill_b(src[16 + 6 *src_stride]);
        inp7_sld2 = (v16u8)__msa_fill_b(src[16 + 7 *src_stride]);
        inp8_sld2 = (v16u8)__msa_fill_b(src[16 + 8 *src_stride]);
        src += 8 * src_stride;

        SLDI_B2_UB(inp0_sld2, inp1_sld2, inp0, inp1, inp0_sld2, inp1_sld2, 1);
        SLDI_B2_UB(inp2_sld2, inp3_sld2, inp2, inp3, inp2_sld2, inp3_sld2, 1);
        SLDI_B2_UB(inp4_sld2, inp5_sld2, inp4, inp5, inp4_sld2, inp5_sld2, 1);
        SLDI_B2_UB(inp6_sld2, inp7_sld2, inp6, inp7, inp6_sld2, inp7_sld2, 1);
        inp8_sld2 = (v16u8)__msa_sldi_b((v16i8)inp8_sld2, (v16i8)inp8, 1);

        ILVR_B8_UH(inp0_sld2, inp0, inp1_sld2, inp1, inp2_sld2, inp2,
                   inp3_sld2, inp3, inp4_sld2, inp4, inp5_sld2, inp5,
                   inp6_sld2, inp6, inp7_sld2, inp7, src0, src1, src2,
                   src3, src4, src5, src6, src7);
        src8 = (v8u16)__msa_ilvr_b((v16i8)inp8_sld2, (v16i8)inp8);

        ILVL_B4_UH(inp0_sld2, inp0, inp1_sld2, inp1, inp2_sld2, inp2,
                   inp3_sld2, inp3, src9, src10, src11, src12);
        ILVL_B4_UH(inp4_sld2, inp4, inp5_sld2, inp5, inp6_sld2, inp6,
                   inp7_sld2, inp7, src13, src14, src15, src16);
        src17 = (v8u16)__msa_ilvl_b((v16i8)inp8_sld2, (v16i8)inp8);

        HADD_UB4_UH(src0, src1, src2, src3, src0_sld2, src1_sld2, src2_sld2,
                    src3_sld2);
        HADD_UB4_UH(src4, src5, src6, src7, src4_sld2, src5_sld2, src6_sld2,
                    src7_sld2);
        src8_sld2 = __msa_hadd_u_h((v16u8)src8, (v16u8)src8);

        HADD_UB4_UH(src9, src10, src11, src12, src9_sld2, src10_sld2,
                    src11_sld2, src12_sld2);
        HADD_UB4_UH(src13, src14, src15, src16, src13_sld2, src14_sld2,
                    src15_sld2, src16_sld2);
        src17_sld2 = __msa_hadd_u_h((v16u8)src17, (v16u8)src17);

        sum0 = src0_sld2 + src1_sld2;
        sum1 = src1_sld2 + src2_sld2;
        sum2 = src2_sld2 + src3_sld2;
        sum3 = src3_sld2 + src4_sld2;
        sum4 = src4_sld2 + src5_sld2;
        sum5 = src5_sld2 + src6_sld2;
        sum6 = src6_sld2 + src7_sld2;
        sum7 = src7_sld2 + src8_sld2;
        sum8 = src9_sld2 + src10_sld2;
        sum9 = src10_sld2 + src11_sld2;
        sum10 = src11_sld2 + src12_sld2;
        sum11 = src12_sld2 + src13_sld2;
        sum12 = src13_sld2 + src14_sld2;
        sum13 = src14_sld2 + src15_sld2;
        sum14 = src15_sld2 + src16_sld2;
        sum15 = src16_sld2 + src17_sld2;

        SRARI_H4_UH(sum0, sum1, sum2, sum3, 2);
        SRARI_H4_UH(sum4, sum5, sum6, sum7, 2);
        SRARI_H4_UH(sum8, sum9, sum10, sum11, 2);
        SRARI_H4_UH(sum12, sum13, sum14, sum15, 2);

        PCKEV_B2_SB(sum8, sum0, sum9, sum1, tmp0, tmp1);
        PCKEV_B2_SB(sum10, sum2, sum11, sum3, tmp2, tmp3);
        ST_SB4(tmp0, tmp1, tmp2, tmp3, dst, dst_stride);
        dst += 4 * dst_stride;

        PCKEV_B2_SB(sum12, sum4, sum13, sum5, tmp0, tmp1);
        PCKEV_B2_SB(sum14, sum6, sum15, sum7, tmp2, tmp3);
        ST_SB4(tmp0, tmp1, tmp2, tmp3, dst, dst_stride);
        dst += 4 * dst_stride;
    }
}

static void copy_16x8_msa(UWORD8 *src, WORD32 src_stride,
                          UWORD8 *dst, WORD32 dst_stride)
{
    v16u8 src0, src1, src2, src3, src4, src5, src6, src7;

    LD_UB8(src, src_stride, src0, src1, src2, src3, src4, src5, src6, src7);
    ST_UB8(src0, src1, src2, src3, src4, src5, src6, src7, dst, dst_stride);
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

void impeg2d_mc_halfx_halfy_msa(void *pv_dec, UWORD8 *ref, UWORD32 ref_stride,
                                UWORD8 *dst, UWORD32 dst_stride,
                                UWORD32 blk_width, UWORD32 blk_height)
{
    dec_state_t *ps_dec = (dec_state_t *)pv_dec;

    if((MB_SIZE == blk_width) && (MB_SIZE == blk_height))
    {
        m4v_h263_hv_filter_16width_msa(ref, ref_stride, dst, dst_stride,
                                       MB_SIZE);
    }
    else if((BLK_SIZE == blk_width) && (BLK_SIZE == blk_height))
    {
        ps_dec->pf_halfx_halfy_8x8(dst, ref, ref_stride, dst_stride);
    }
    else if((MB_SIZE == blk_width) && (BLK_SIZE == blk_height))
    {
        m4v_h263_hv_filter_16width_msa(ref, ref_stride, dst, dst_stride,
                                       BLK_SIZE);
    }
    else
    {
        UWORD8 *ref_p0, *ref_p1, *ref_p2, *ref_p3;
        UWORD32 i, j;

        ref_p0 = ref;
        ref_p1 = ref + 1;
        ref_p2 = ref + ref_stride;
        ref_p3 = ref + ref_stride + 1;

        for(i = blk_height; i--;)
        {
            for(j = blk_width; j--;)
            {
                *dst++ = (((*ref_p0++) + (*ref_p1++) + (*ref_p2++) +
                           (*ref_p3++) + 2) >> 2);
            }

            ref_p0 += ref_stride - blk_width;
            ref_p1 += ref_stride - blk_width;
            ref_p2 += ref_stride - blk_width;
            ref_p3 += ref_stride - blk_width;
            dst += dst_stride - blk_width;
        }
    }

    return;
}

void impeg2d_mc_halfx_fully_msa(void *pv_dec, UWORD8 *ref,
                                UWORD32 ref_stride, UWORD8 *dst,
                                UWORD32 dst_stride, UWORD32 blk_width,
                                UWORD32 blk_height)
{
    dec_state_t *ps_dec = (dec_state_t *)pv_dec;

    if((MB_SIZE == blk_width) && (MB_SIZE == blk_height))
    {
        m4v_h263_horiz_filter_16width_msa(ref, ref_stride, dst, dst_stride,
                                          MB_SIZE);
    }
    else if((BLK_SIZE == blk_width) && (BLK_SIZE == blk_height))
    {
        ps_dec->pf_halfx_fully_8x8(dst, ref, ref_stride, dst_stride);
    }
    else if((MB_SIZE == blk_width) && (BLK_SIZE == blk_height))
    {
        m4v_h263_horiz_filter_16width_msa(ref, ref_stride, dst, dst_stride,
                                          BLK_SIZE);
    }
    else
    {
        UWORD8 *ref_p0, *ref_p1;
        UWORD32 i, j;

        ref_p0 = ref;
        ref_p1 = ref + 1;

        for(i = blk_height; i--;)
        {
            for(j = blk_width; j--;)
            {
                *dst++ = (((*ref_p0++) + (*ref_p1++) + 1) >> 1);
            }

            ref_p0 += ref_stride - blk_width;
            ref_p1 += ref_stride - blk_width;
            dst += dst_stride - blk_width;
        }
    }

    return;
}

void impeg2d_mc_fullx_halfy_msa(void *pv_dec, UWORD8 *ref,
                                UWORD32 ref_stride, UWORD8 *dst,
                                UWORD32 dst_stride, UWORD32 blk_width,
                                UWORD32 blk_height)
{
    dec_state_t *ps_dec = (dec_state_t *)pv_dec;

    if((MB_SIZE == blk_width) && (MB_SIZE == blk_height))
    {
        m4v_h263_vert_filter_16width_msa(ref, ref_stride, dst, dst_stride,
                                         MB_SIZE);
    }
    else if((BLK_SIZE == blk_width) && (BLK_SIZE == blk_height))
    {
        ps_dec->pf_fullx_halfy_8x8(dst, ref, ref_stride, dst_stride);
    }
    else if((MB_SIZE == blk_width) && (BLK_SIZE == blk_height))
    {
        m4v_h263_vert_filter_16width_msa(ref, ref_stride, dst, dst_stride,
                                         BLK_SIZE);
    }
    else
    {
        UWORD8 *ref_p0, *ref_p1;
        UWORD32 i, j;

        ref_p0 = ref;
        ref_p1 = ref + ref_stride;

        for(i = blk_height; i--;)
        {
            for(j = blk_width; j--;)
            {
                *dst++ = (((*ref_p0++) + (*ref_p1++) + 1) >> 1);
            }

            ref_p0 += ref_stride - blk_width;
            ref_p1 += ref_stride - blk_width;
            dst += dst_stride - blk_width;
        }
    }

    return;
}

void impeg2d_mc_fullx_fully_msa(void *pv_dec, UWORD8 *ref,
                                UWORD32 ref_stride, UWORD8 *dst,
                                UWORD32 dst_stride, UWORD32 blk_width,
                                UWORD32 blk_height)
{
    dec_state_t *ps_dec = (dec_state_t *)pv_dec;

    if((MB_SIZE == blk_width) && (MB_SIZE == blk_height))
    {
        copy_16x16_msa(ref, ref_stride, dst, dst_stride);
    }
    else if((BLK_SIZE == blk_width) && (BLK_SIZE == blk_height))
    {
        ps_dec->pf_fullx_fully_8x8(dst, ref, ref_stride, dst_stride);
    }
    else if((MB_SIZE == blk_width) && (BLK_SIZE == blk_height))
    {
        copy_16x8_msa(ref, ref_stride, dst, dst_stride);
    }
    else
    {
        UWORD32 i;

        for(i = blk_height; i--;)
        {
            memcpy(dst, ref, blk_width);

            ref += ref_stride;
            dst += dst_stride;
        }
    }

    return;
}
