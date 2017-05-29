project('opus', 'c', default_options: ['c_std=c11'])

opus_head = [
	'opus/include/opus.h',
	'opus/include/opus_multistream.h',
	'opus/src/opus_private.h',
	'opus/src/analysis.h',
	'opus/src/mlp.h',
	'opus/src/tansig_table.h',
]

opus_sources = [
	'opus/src/opus.c',
	'opus/src/opus_decoder.c',
	'opus/src/opus_encoder.c',
	'opus/src/opus_multistream.c',
	'opus/src/opus_multistream_encoder.c',
	'opus/src/opus_multistream_decoder.c',
	'opus/src/repacketizer.c',
]

opus_sources_float = [
	'opus/src/analysis.c',
	'opus/src/mlp.c',
	'opus/src/mlp_data.c',
]

silk_head = [
	'opus/silk/debug.h',
	'opus/silk/control.h',
	'opus/silk/errors.h',
	'opus/silk/API.h',
	'opus/silk/typedef.h',
	'opus/silk/define.h',
	'opus/silk/main.h',
	'opus/silk/x86/main_sse.h',
	'opus/silk/PLC.h',
	'opus/silk/structs.h',
	'opus/silk/tables.h',
	'opus/silk/tuning_parameters.h',
	'opus/silk/Inlines.h',
	'opus/silk/MacroCount.h',
	'opus/silk/MacroDebug.h',
	'opus/silk/macros.h',
	'opus/silk/NSQ.h',
	'opus/silk/pitch_est_defines.h',
	'opus/silk/resampler_private.h',
	'opus/silk/resampler_rom.h',
	'opus/silk/resampler_structs.h',
	'opus/silk/SigProc_FIX.h',
	'opus/silk/x86/SigProc_FIX_sse.h',
	'opus/silk/arm/biquad_alt_arm.h',
	'opus/silk/arm/LPC_inv_pred_gain_arm.h',
	'opus/silk/arm/macros_armv4.h',
	'opus/silk/arm/macros_armv5e.h',
	'opus/silk/arm/macros_arm64.h',
	'opus/silk/arm/SigProc_FIX_armv4.h',
	'opus/silk/arm/SigProc_FIX_armv5e.h',
	'opus/silk/arm/NSQ_del_dec_arm.h',
	'opus/silk/arm/NSQ_neon.h',
	'opus/silk/fixed/main_FIX.h',
	'opus/silk/fixed/structs_FIX.h',
	'opus/silk/fixed/arm/warped_autocorrelation_FIX_arm.h',
	'opus/silk/fixed/mips/noise_shape_analysis_FIX_mipsr1.h',
	'opus/silk/fixed/mips/warped_autocorrelation_FIX_mipsr1.h',
	'opus/silk/float/main_FLP.h',
	'opus/silk/float/structs_FLP.h',
	'opus/silk/float/SigProc_FLP.h',
	'opus/silk/mips/macros_mipsr1.h',
	'opus/silk/mips/NSQ_del_dec_mipsr1.h',
	'opus/silk/mips/sigproc_fix_mipsr1.h',
]

silk_sources = [
	'opus/silk/CNG.c',
	'opus/silk/code_signs.c',
	'opus/silk/init_decoder.c',
	'opus/silk/decode_core.c',
	'opus/silk/decode_frame.c',
	'opus/silk/decode_parameters.c',
	'opus/silk/decode_indices.c',
	'opus/silk/decode_pulses.c',
	'opus/silk/decoder_set_fs.c',
	'opus/silk/dec_API.c',
	'opus/silk/enc_API.c',
	'opus/silk/encode_indices.c',
	'opus/silk/encode_pulses.c',
	'opus/silk/gain_quant.c',
	'opus/silk/interpolate.c',
	'opus/silk/LP_variable_cutoff.c',
	'opus/silk/NLSF_decode.c',
	'opus/silk/NSQ.c',
	'opus/silk/NSQ_del_dec.c',
	'opus/silk/PLC.c',
	'opus/silk/shell_coder.c',
	'opus/silk/tables_gain.c',
	'opus/silk/tables_LTP.c',
	'opus/silk/tables_NLSF_CB_NB_MB.c',
	'opus/silk/tables_NLSF_CB_WB.c',
	'opus/silk/tables_other.c',
	'opus/silk/tables_pitch_lag.c',
	'opus/silk/tables_pulses_per_block.c',
	'opus/silk/VAD.c',
	'opus/silk/control_audio_bandwidth.c',
	'opus/silk/quant_LTP_gains.c',
	'opus/silk/VQ_WMat_EC.c',
	'opus/silk/HP_variable_cutoff.c',
	'opus/silk/NLSF_encode.c',
	'opus/silk/NLSF_VQ.c',
	'opus/silk/NLSF_unpack.c',
	'opus/silk/NLSF_del_dec_quant.c',
	'opus/silk/process_NLSFs.c',
	'opus/silk/stereo_LR_to_MS.c',
	'opus/silk/stereo_MS_to_LR.c',
	'opus/silk/check_control_input.c',
	'opus/silk/control_SNR.c',
	'opus/silk/init_encoder.c',
	'opus/silk/control_codec.c',
	'opus/silk/A2NLSF.c',
	'opus/silk/ana_filt_bank_1.c',
	'opus/silk/biquad_alt.c',
	'opus/silk/bwexpander_32.c',
	'opus/silk/bwexpander.c',
	'opus/silk/debug.c',
	'opus/silk/decode_pitch.c',
	'opus/silk/inner_prod_aligned.c',
	'opus/silk/lin2log.c',
	'opus/silk/log2lin.c',
	'opus/silk/LPC_analysis_filter.c',
	'opus/silk/LPC_inv_pred_gain.c',
	'opus/silk/table_LSF_cos.c',
	'opus/silk/NLSF2A.c',
	'opus/silk/NLSF_stabilize.c',
	'opus/silk/NLSF_VQ_weights_laroia.c',
	'opus/silk/pitch_est_tables.c',
	'opus/silk/resampler.c',
	'opus/silk/resampler_down2_3.c',
	'opus/silk/resampler_down2.c',
	'opus/silk/resampler_private_AR2.c',
	'opus/silk/resampler_private_down_FIR.c',
	'opus/silk/resampler_private_IIR_FIR.c',
	'opus/silk/resampler_private_up2_HQ.c',
	'opus/silk/resampler_rom.c',
	'opus/silk/sigm_Q15.c',
	'opus/silk/sort.c',
	'opus/silk/sum_sqr_shift.c',
	'opus/silk/stereo_decode_pred.c',
	'opus/silk/stereo_encode_pred.c',
	'opus/silk/stereo_find_predictor.c',
	'opus/silk/stereo_quant_pred.c',
	'opus/silk/LPC_fit.c',
]

silk_sources_sse4_1 = [
	'opus/silk/x86/NSQ_sse.c',
	'opus/silk/x86/NSQ_del_dec_sse.c',
	'opus/silk/x86/x86_silk_map.c',
	'opus/silk/x86/VAD_sse.c',
	'opus/silk/x86/VQ_WMat_EC_sse.c',
]

silk_sources_arm_neon_intr = [
	'opus/silk/arm/arm_silk_map.c',
	'opus/silk/arm/biquad_alt_neon_intr.c',
	'opus/silk/arm/LPC_inv_pred_gain_neon_intr.c',
	'opus/silk/arm/NSQ_del_dec_neon_intr.c',
	'opus/silk/arm/NSQ_neon.c',
]

silk_sources_fixed = [
	'opus/silk/fixed/LTP_analysis_filter_FIX.c',
	'opus/silk/fixed/LTP_scale_ctrl_FIX.c',
	'opus/silk/fixed/corrMatrix_FIX.c',
	'opus/silk/fixed/encode_frame_FIX.c',
	'opus/silk/fixed/find_LPC_FIX.c',
	'opus/silk/fixed/find_LTP_FIX.c',
	'opus/silk/fixed/find_pitch_lags_FIX.c',
	'opus/silk/fixed/find_pred_coefs_FIX.c',
	'opus/silk/fixed/noise_shape_analysis_FIX.c',
	'opus/silk/fixed/process_gains_FIX.c',
	'opus/silk/fixed/regularize_correlations_FIX.c',
	'opus/silk/fixed/residual_energy16_FIX.c',
	'opus/silk/fixed/residual_energy_FIX.c',
	'opus/silk/fixed/warped_autocorrelation_FIX.c',
	'opus/silk/fixed/apply_sine_window_FIX.c',
	'opus/silk/fixed/autocorr_FIX.c',
	'opus/silk/fixed/burg_modified_FIX.c',
	'opus/silk/fixed/k2a_FIX.c',
	'opus/silk/fixed/k2a_Q16_FIX.c',
	'opus/silk/fixed/pitch_analysis_core_FIX.c',
	'opus/silk/fixed/vector_ops_FIX.c',
	'opus/silk/fixed/schur64_FIX.c',
	'opus/silk/fixed/schur_FIX.c',
]

silk_sources_fixed_sse4_1 = [
	'opus/silk/fixed/x86/vector_ops_FIX_sse.c',
	'opus/silk/fixed/x86/burg_modified_FIX_sse.c',
]

silk_sources_fixed_arm_neon_intr = [
	'opus/silk/fixed/arm/warped_autocorrelation_FIX_neon_intr.c',
]

silk_sources_float = [
	'opus/silk/float/apply_sine_window_FLP.c',
	'opus/silk/float/corrMatrix_FLP.c',
	'opus/silk/float/encode_frame_FLP.c',
	'opus/silk/float/find_LPC_FLP.c',
	'opus/silk/float/find_LTP_FLP.c',
	'opus/silk/float/find_pitch_lags_FLP.c',
	'opus/silk/float/find_pred_coefs_FLP.c',
	'opus/silk/float/LPC_analysis_filter_FLP.c',
	'opus/silk/float/LTP_analysis_filter_FLP.c',
	'opus/silk/float/LTP_scale_ctrl_FLP.c',
	'opus/silk/float/noise_shape_analysis_FLP.c',
	'opus/silk/float/process_gains_FLP.c',
	'opus/silk/float/regularize_correlations_FLP.c',
	'opus/silk/float/residual_energy_FLP.c',
	'opus/silk/float/warped_autocorrelation_FLP.c',
	'opus/silk/float/wrappers_FLP.c',
	'opus/silk/float/autocorrelation_FLP.c',
	'opus/silk/float/burg_modified_FLP.c',
	'opus/silk/float/bwexpander_FLP.c',
	'opus/silk/float/energy_FLP.c',
	'opus/silk/float/inner_product_FLP.c',
	'opus/silk/float/k2a_FLP.c',
	'opus/silk/float/LPC_inv_pred_gain_FLP.c',
	'opus/silk/float/pitch_analysis_core_FLP.c',
	'opus/silk/float/scale_copy_vector_FLP.c',
	'opus/silk/float/scale_vector_FLP.c',
	'opus/silk/float/schur_FLP.c',
	'opus/silk/float/sort_FLP.c',
]

celt_head = [
	'opus/celt/arch.h',
	'opus/celt/bands.h',
	'opus/celt/celt.h',
	'opus/celt/cpu_support.h',
	'opus/include/opus_types.h',
	'opus/include/opus_defines.h',
	'opus/include/opus_custom.h',
	'opus/celt/cwrs.h',
	'opus/celt/ecintrin.h',
	'opus/celt/entcode.h',
	'opus/celt/entdec.h',
	'opus/celt/entenc.h',
	'opus/celt/fixed_debug.h',
	'opus/celt/fixed_generic.h',
	'opus/celt/float_cast.h',
	'opus/celt/_kiss_fft_guts.h',
	'opus/celt/kiss_fft.h',
	'opus/celt/laplace.h',
	'opus/celt/mathops.h',
	'opus/celt/mdct.h',
	'opus/celt/mfrngcod.h',
	'opus/celt/modes.h',
	'opus/celt/os_support.h',
	'opus/celt/pitch.h',
	'opus/celt/celt_lpc.h',
	'opus/celt/x86/celt_lpc_sse.h',
	'opus/celt/quant_bands.h',
	'opus/celt/rate.h',
	'opus/celt/stack_alloc.h',
	'opus/celt/vq.h',
	'opus/celt/static_modes_float.h',
	'opus/celt/static_modes_fixed.h',
	'opus/celt/static_modes_float_arm_ne10.h',
	'opus/celt/static_modes_fixed_arm_ne10.h',
	'opus/celt/arm/armcpu.h',
	'opus/celt/arm/fixed_armv4.h',
	'opus/celt/arm/fixed_armv5e.h',
	'opus/celt/arm/fixed_arm64.h',
	'opus/celt/arm/kiss_fft_armv4.h',
	'opus/celt/arm/kiss_fft_armv5e.h',
	'opus/celt/arm/pitch_arm.h',
	'opus/celt/arm/fft_arm.h',
	'opus/celt/arm/mdct_arm.h',
	'opus/celt/mips/celt_mipsr1.h',
	'opus/celt/mips/fixed_generic_mipsr1.h',
	'opus/celt/mips/kiss_fft_mipsr1.h',
	'opus/celt/mips/mdct_mipsr1.h',
	'opus/celt/mips/pitch_mipsr1.h',
	'opus/celt/mips/vq_mipsr1.h',
	'opus/celt/x86/pitch_sse.h',
	'opus/celt/x86/vq_sse.h',
	'opus/celt/x86/x86cpu.h',
]

celt_sources = [
	'opus/celt/bands.c',
	'opus/celt/celt.c',
	'opus/celt/celt_encoder.c',
	'opus/celt/celt_decoder.c',
	'opus/celt/cwrs.c',
	'opus/celt/entcode.c',
	'opus/celt/entdec.c',
	'opus/celt/entenc.c',
	'opus/celt/kiss_fft.c',
	'opus/celt/laplace.c',
	'opus/celt/mathops.c',
	'opus/celt/mdct.c',
	'opus/celt/modes.c',
	'opus/celt/pitch.c',
	'opus/celt/celt_lpc.c',
	'opus/celt/quant_bands.c',
	'opus/celt/rate.c',
	'opus/celt/vq.c',
]

celt_sources_sse = [
	'opus/celt/x86/x86cpu.c',
	'opus/celt/x86/x86_celt_map.c',
	'opus/celt/x86/pitch_sse.c',
]

celt_sources_sse2 = [
	'opus/celt/x86/pitch_sse2.c',
	'opus/celt/x86/vq_sse2.c',
]

celt_sources_sse4_1 = [
	'opus/celt/x86/celt_lpc_sse.c',
	'opus/celt/x86/pitch_sse4_1.c',
]

celt_sources_arm = [
	'opus/celt/arm/armcpu.c',
	'opus/celt/arm/arm_celt_map.c',
]

celt_sources_arm_asm = [
	'opus/celt/arm/celt_pitch_xcorr_arm.s',
]

celt_am_sources_arm_asm = [
	'opus/celt/arm/armopts.s.in',
]

celt_sources_arm_neon_intr = [
	'opus/celt/arm/celt_neon_intr.c',
]

celt_sources_arm_ne10 = [
	'opus/celt/arm/celt_ne10_fft.c',
	'opus/celt/arm/celt_ne10_mdct.c',
]

# I need:
#
# opus_head, opus_sources, opus_sources_float,
# silk_head, silk_sources, silk_sources_sse4_1, silk_sources_float,
# celt_head, celt_sources, celt_sources_sse, celt_sources_sse2, celt_sources_sse4_1,

opus_inc = include_directories('opus')
opus_lib = library('opus', [
	opus_head,
	opus_sources,
	opus_sources_float,
	silk_head,
	silk_sources,
	silk_sources_sse4_1,
	silk_sources_float,
	celt_head,
	celt_sources,
	celt_sources_sse,
	celt_sources_sse2,
	celt_sources_sse4_1,
])

opus = declare_dependency(link_with: opus_lib, include_directories: opus_inc)



