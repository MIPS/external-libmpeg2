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
#include "impeg2_macros_msa.h"

void impeg2_memset_8bit_8x8_block_msa(UWORD8 *dst, WORD32 dc_val,
                                      WORD32 dst_stride)
{
    uint64_t value_64bit = dc_val * 0x0101010101010101;

    SD4(value_64bit, value_64bit, value_64bit, value_64bit, dst, dst_stride);
    dst += (4 * dst_stride);
    SD4(value_64bit, value_64bit, value_64bit, value_64bit, dst, dst_stride);
}

void impeg2_memset0_16bit_8x8_linear_block_msa(WORD16 *buf)
{
    v8u16 zero = { 0 };

    ST_UH8(zero, zero, zero, zero, zero, zero, zero, zero, buf, 8);
}
