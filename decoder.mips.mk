libmpeg2d_inc_dir_mips  +=  $(LOCAL_PATH)/common/mips

libmpeg2d_srcs_c_mips   +=  decoder/mips/impeg2d_function_selector.c
libmpeg2d_srcs_c_mips   +=  common/mips/ideint_function_selector.c

ifeq ($(ARCH_MIPS_HAS_MSA),true)
libmpeg2d_cflags_mips   += -mfp64 -mmsa
libmpeg2d_cflags_mips   += -DDEFAULT_ARCH=D_ARCH_MIPS_MSA

libmpeg2d_srcs_c_mips   +=  decoder/mips/impeg2d_function_selector_msa.c
libmpeg2d_srcs_c_mips   +=  decoder/mips/impeg2d_mc_msa.c
libmpeg2d_srcs_c_mips   +=  common/mips/impeg2_format_conv_msa.c
libmpeg2d_srcs_c_mips   +=  common/mips/impeg2_idct_msa.c
libmpeg2d_srcs_c_mips   +=  common/mips/impeg2_inter_pred_msa.c
libmpeg2d_srcs_c_mips   +=  common/mips/impeg2_mem_func_msa.c
else
libmpeg2d_cflags_mips   += -DDISABLE_MSA -DDEFAULT_ARCH=D_ARCH_MIPS_NOMSA
endif

LOCAL_C_INCLUDES_mips   += $(libmpeg2d_inc_dir_mips)
LOCAL_SRC_FILES_mips    += $(libmpeg2d_srcs_c_mips)
LOCAL_CFLAGS_mips       += $(libmpeg2d_cflags_mips)
