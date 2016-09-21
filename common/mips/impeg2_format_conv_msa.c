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
#include "impeg2_format_conv.h"
#include "impeg2_macros_msa.h"

static void copy_width8_msa(UWORD8 *src, WORD32 src_stride,
                            UWORD8 *dst, WORD32 dst_stride,
                            WORD32 height)
{
    WORD32 cnt;
    uint64_t out0, out1, out2, out3, out4, out5, out6, out7;

    if(0 == height % 12)
    {
        for(cnt = (height / 12); cnt--;)
        {
            LD4(src, src_stride, out0, out1, out2, out3);
            src += (4 * src_stride);
            LD4(src, src_stride, out4, out5, out6, out7);
            src += (4 * src_stride);

            SD4(out0, out1, out2, out3, dst, dst_stride);
            dst += (4 * dst_stride);
            SD4(out4, out5, out6, out7, dst, dst_stride);
            dst += (4 * dst_stride);

            LD4(src, src_stride, out0, out1, out2, out3);
            src += (4 * src_stride);

            SD4(out0, out1, out2, out3, dst, dst_stride);
            dst += (4 * dst_stride);
        }
    }
    else if(0 == height % 8)
    {
        for(cnt = height >> 3; cnt--;)
        {
            LD4(src, src_stride, out0, out1, out2, out3);
            src += (4 * src_stride);
            LD4(src, src_stride, out4, out5, out6, out7);
            src += (4 * src_stride);

            SD4(out0, out1, out2, out3, dst, dst_stride);
            dst += (4 * dst_stride);
            SD4(out4, out5, out6, out7, dst, dst_stride);
            dst += (4 * dst_stride);
        }
    }
    else if(0 == height % 4)
    {
        for(cnt = (height / 4); cnt--;)
        {
            LD4(src, src_stride, out0, out1, out2, out3);
            src += (4 * src_stride);

            SD4(out0, out1, out2, out3, dst, dst_stride);
            dst += (4 * dst_stride);
        }
    }
    else if(0 == height % 2)
    {
        for(cnt = (height / 2); cnt--;)
        {
            out0 = LD(src);
            src += src_stride;
            out1 = LD(src);
            src += src_stride;

            SD(out0, dst);
            dst += dst_stride;
            SD(out1, dst);
            dst += dst_stride;
        }
    }
}

static void copy_16multx8mult_msa(UWORD8 *src, WORD32 src_stride,
                                  UWORD8 *dst, WORD32 dst_stride,
                                  WORD32 height, WORD32 width)
{
    WORD32 cnt, loop_cnt;
    UWORD8 *src_tmp, *dst_tmp;
    v16u8 src0, src1, src2, src3, src4, src5, src6, src7;

    for(cnt = (width >> 4); cnt--;)
    {
        src_tmp = src;
        dst_tmp = dst;

        for(loop_cnt = (height >> 3); loop_cnt--;)
        {
            LD_UB8(src_tmp, src_stride,
                   src0, src1, src2, src3, src4, src5, src6, src7);
            src_tmp += (8 * src_stride);

            ST_UB8(src0, src1, src2, src3, src4, src5, src6, src7,
                   dst_tmp, dst_stride);
            dst_tmp += (8 * dst_stride);
        }

        src += 16;
        dst += 16;
    }
}

static void plane_copy_interleave_msa(UWORD8 *src_ptr0, WORD32 src0_stride,
                                      UWORD8 *src_ptr1, WORD32 src1_stride,
                                      UWORD8 *dst, WORD32 dst_stride,
                                      WORD32 width, WORD32 height)
{
    WORD32 loop_width, loop_height, w_mul8, h;
    v16u8 src0, src1, src2, src3, src4, src5, src6, src7;
    v16u8 vec_ilv_r0, vec_ilv_r1, vec_ilv_r2, vec_ilv_r3;
    v16u8 vec_ilv_l0, vec_ilv_l1, vec_ilv_l2, vec_ilv_l3;

    w_mul8 = width - width % 8;
    h = height - height % 4;

    for(loop_height = (h >> 2); loop_height--;)
    {
        for(loop_width = (width >> 4); loop_width--;)
        {
            LD_UB4(src_ptr0, src0_stride, src0, src1, src2, src3);
            LD_UB4(src_ptr1, src1_stride, src4, src5, src6, src7);
            ILVR_B4_UB(src4, src0, src5, src1, src6, src2, src7, src3,
                       vec_ilv_r0, vec_ilv_r1, vec_ilv_r2, vec_ilv_r3);
            ILVL_B4_UB(src4, src0, src5, src1, src6, src2, src7, src3,
                       vec_ilv_l0, vec_ilv_l1, vec_ilv_l2, vec_ilv_l3);
            ST_UB4(vec_ilv_r0, vec_ilv_r1, vec_ilv_r2, vec_ilv_r3,
                   dst, dst_stride);
            ST_UB4(vec_ilv_l0, vec_ilv_l1, vec_ilv_l2, vec_ilv_l3,
                   (dst + 16), dst_stride);
            src_ptr0 += 16;
            src_ptr1 += 16;
            dst += 32;
        }

        for(loop_width = (width % 16) >> 3; loop_width--;)
        {
            LD_UB4(src_ptr0, src0_stride, src0, src1, src2, src3);
            LD_UB4(src_ptr1, src1_stride, src4, src5, src6, src7);
            ILVR_B4_UB(src4, src0, src5, src1, src6, src2, src7, src3,
                       vec_ilv_r0, vec_ilv_r1, vec_ilv_r2, vec_ilv_r3);
            ST_UB4(vec_ilv_r0, vec_ilv_r1, vec_ilv_r2, vec_ilv_r3,
                   dst, dst_stride);
            src_ptr0 += 8;
            src_ptr1 += 8;
            dst += 16;
        }

        for(loop_width = w_mul8; loop_width < width; loop_width++)
        {
            dst[0] = src_ptr0[0];
            dst[1] = src_ptr1[0];
            dst[dst_stride] = src_ptr0[src0_stride];
            dst[dst_stride + 1] = src_ptr1[src1_stride];
            dst[2 * dst_stride] = src_ptr0[2 * src0_stride];
            dst[2 * dst_stride + 1] = src_ptr1[2 * src1_stride];
            dst[3 * dst_stride] = src_ptr0[3 * src0_stride];
            dst[3 * dst_stride + 1] = src_ptr1[3 * src1_stride];
            src_ptr0 += 1;
            src_ptr1 += 1;
            dst += 2;
        }

        src_ptr0 += ((4 * src0_stride) - width);
        src_ptr1 += ((4 * src1_stride) - width);
        dst += ((4 * dst_stride) - (width * 2));
    }

    for(loop_height = h; loop_height < height; loop_height++)
    {
        for(loop_width = (width >> 4); loop_width--;)
        {
            src0 = LD_UB(src_ptr0);
            src4 = LD_UB(src_ptr1);
            ILVRL_B2_UB(src4, src0, vec_ilv_r0, vec_ilv_l0);
            ST_UB2(vec_ilv_r0, vec_ilv_l0, dst, 16);
            src_ptr0 += 16;
            src_ptr1 += 16;
            dst += 32;
        }

        for(loop_width = (width % 16) >> 3; loop_width--;)
        {
            src0 = LD_UB(src_ptr0);
            src4 = LD_UB(src_ptr1);
            vec_ilv_r0 = (v16u8)__msa_ilvr_b((v16i8)src4, (v16i8)src0);
            ST_UB(vec_ilv_r0, dst);
            src_ptr0 += 8;
            src_ptr1 += 8;
            dst += 16;
        }

        for(loop_width = w_mul8; loop_width < width; loop_width++)
        {
            dst[0] = src_ptr0[0];
            dst[1] = src_ptr1[0];
            src_ptr0 += 1;
            src_ptr1 += 1;
            dst += 2;
        }

        src_ptr0 += (src0_stride - width);
        src_ptr1 += (src1_stride - width);
        dst += (dst_stride - (width * 2));
    }
}

void impeg2_copy_frm_yuv420p_msa(UWORD8 *src_y, UWORD8 *src_u, UWORD8 *src_v,
                                 UWORD8 *dst_y, UWORD8 *dst_u, UWORD8 *dst_v,
                                 UWORD32 width, UWORD32 height,
                                 UWORD32 src_stride_y, UWORD32 src_stride_u,
                                 UWORD32 src_stride_v, UWORD32 dst_stride_y,
                                 UWORD32 dst_stride_u, UWORD32 dst_stride_v)
{
    WORD32 uv_height = height >> 1;
    WORD32 uv_width = width >> 1;
    WORD32 w8 = uv_width - (uv_width % 16);

    copy_16multx8mult_msa(src_y, src_stride_y, dst_y, dst_stride_y,
                          (WORD32) height, width);

    copy_16multx8mult_msa(src_u, src_stride_u, dst_u, dst_stride_u,
                          uv_height, uv_width);
    if(0 != (uv_width % 16))
    {
        copy_width8_msa(src_u + w8, src_stride_u, dst_u + w8, dst_stride_u,
                        uv_height);
    }

    copy_16multx8mult_msa(src_v, src_stride_v, dst_v, dst_stride_v,
                          uv_height, uv_width);
    if(0 != (uv_width % 16))
    {
        copy_width8_msa(src_v + w8, src_stride_v, dst_v + w8, dst_stride_v,
                        uv_height);
    }
}

void impeg2_fmt_conv_yuv420p_to_yuv420sp_uv_msa(UWORD8 *src_y, UWORD8 *src_u,
                                                UWORD8 *src_v, UWORD8 *dst_y,
                                                UWORD8 *dst_uv, UWORD32 height,
                                                UWORD32 width, UWORD32 stridey,
                                                UWORD32 strideu,
                                                UWORD32 stridev,
                                                UWORD32 dst_stride_y,
                                                UWORD32 dst_stride_uv,
                                                UWORD32 convert_uv_only)

{
    UWORD32 width_uv;

    if(0 == convert_uv_only)
    {
        copy_16multx8mult_msa(src_y, stridey, dst_y, dst_stride_y,
                              height, width);
    }

    height = (height + 1) >> 1;
    width_uv = (width + 1) >> 1;
    plane_copy_interleave_msa(src_u, strideu, src_v, stridev,
                              dst_uv, dst_stride_uv, width_uv,
                              height);
}

void impeg2_fmt_conv_yuv420p_to_yuv420sp_vu_msa(UWORD8 *src_y, UWORD8 *src_u,
                                                UWORD8 *src_v, UWORD8 *dst_y,
                                                UWORD8 *dst_uv, UWORD32 height,
                                                UWORD32 width, UWORD32 stridey,
                                                UWORD32 strideu,
                                                UWORD32 stridev,
                                                UWORD32 dst_stride_y,
                                                UWORD32 dst_stride_uv,
                                                UWORD32 convert_uv_only)
{
    UWORD32 width_uv;

    if(0 == convert_uv_only)
    {
        copy_16multx8mult_msa(src_y, stridey, dst_y, dst_stride_y,
                              height, width);
    }

    height = (height + 1) >> 1;
    width_uv = (width + 1) >> 1;
    plane_copy_interleave_msa(src_v, stridev, src_u, strideu,
                              dst_uv, dst_stride_uv, width_uv,
                              height);
}
