PROJECT_NAME     := Smart_Remote_3_nRF52_Firmware_nRF52832
TARGETS          := PCA63519-SR3_nRF52832_Shield CUSTOM-SR3_nRF52832_Custom_Board PCA20023-SR3_nRF52832_Product_Example
OUTPUT_DIRECTORY := _build

SDK_ROOT := ../../../../../..
PROJ_DIR := ../../..

$(OUTPUT_DIRECTORY)/PCA63519-SR3_nRF52832_Shield.out: \
  LINKER_SCRIPT  := Smart_Remote_3_nRF52_gcc_nrf52.ld
$(OUTPUT_DIRECTORY)/CUSTOM-SR3_nRF52832_Custom_Board.out: \
  LINKER_SCRIPT  := Smart_Remote_3_nRF52_gcc_nrf52.ld
$(OUTPUT_DIRECTORY)/PCA20023-SR3_nRF52832_Product_Example.out: \
  LINKER_SCRIPT  := Smart_Remote_3_nRF52_gcc_nrf52.ld

# Source files common to all targets
SRC_FILES += \
  $(PROJ_DIR)/Source/Drivers/drv_acc_bma222e.c \
  $(PROJ_DIR)/Source/Drivers/drv_acc_lis3dh.c \
  $(PROJ_DIR)/Source/Drivers/drv_audio_anr.c \
  $(PROJ_DIR)/Source/Drivers/drv_audio_codec_adpcm.c \
  $(PROJ_DIR)/Source/Drivers/drv_audio_codec_bv32fp.c \
  $(PROJ_DIR)/Source/Drivers/drv_audio_codec_opus.c \
  $(PROJ_DIR)/Source/Drivers/drv_audio_codec_sbc.c \
  $(PROJ_DIR)/Source/Drivers/drv_audio_dsp.c \
  $(PROJ_DIR)/Source/Drivers/drv_audio_pdm.c \
  $(PROJ_DIR)/Source/Drivers/drv_board.c \
  $(PROJ_DIR)/Source/Drivers/drv_buzzer.c \
  $(PROJ_DIR)/Source/Drivers/drv_gyro_icm20608.c \
  $(PROJ_DIR)/Source/Drivers/drv_ir_nec.c \
  $(PROJ_DIR)/Source/Drivers/drv_ir_sirc.c \
  $(PROJ_DIR)/Source/Drivers/drv_keyboard_matrix.c \
  $(PROJ_DIR)/Source/Drivers/drv_keyboard_sx1509.c \
  $(PROJ_DIR)/Source/Drivers/drv_leds_gpio.c \
  $(PROJ_DIR)/Source/Drivers/drv_touchpad_synaptics.c \
  $(SDK_ROOT)/components/drivers_nrf/ble_flash/ble_flash.c \
  $(SDK_ROOT)/components/drivers_nrf/clock/nrf_drv_clock.c \
  $(SDK_ROOT)/components/drivers_nrf/common/nrf_drv_common.c \
  $(SDK_ROOT)/components/drivers_nrf/gpiote/nrf_drv_gpiote.c \
  $(SDK_ROOT)/components/drivers_nrf/pdm/nrf_drv_pdm.c \
  $(SDK_ROOT)/components/drivers_nrf/ppi/nrf_drv_ppi.c \
  $(SDK_ROOT)/components/drivers_nrf/pwm/nrf_drv_pwm.c \
  $(SDK_ROOT)/components/drivers_nrf/rng/nrf_drv_rng.c \
  $(SDK_ROOT)/components/drivers_nrf/saadc/nrf_drv_saadc.c \
  $(SDK_ROOT)/components/drivers_nrf/twi_master/nrf_drv_twi.c \
  $(SDK_ROOT)/components/drivers_nrf/uart/nrf_drv_uart.c \
  $(SDK_ROOT)/components/drivers_nrf/wdt/nrf_drv_wdt.c \
  $(SDK_ROOT)/components/drivers_nrf/hal/nrf_saadc.c \
  $(PROJ_DIR)/Source/Libraries/JLINK_MONITOR_ISR_SES.s \
  $(PROJ_DIR)/main.c \
  $(PROJ_DIR)/Source/Configuration/sr3_config.c \
  $(PROJ_DIR)/Source/Libraries/dvi_adpcm.c \
  $(SDK_ROOT)/components/ble/ble_services/ble_bas/ble_bas.c \
  $(SDK_ROOT)/components/ble/ble_services/ble_dis/ble_dis.c \
  $(SDK_ROOT)/components/ble/ble_services/ble_hids/ble_hids.c \
  $(SDK_ROOT)/components/ble/ble_services/ble_ias/ble_ias.c \
  $(SDK_ROOT)/components/ble/ble_services/ble_nus/ble_nus.c \
  $(PROJ_DIR)/Source/Libraries/bv32fp-1.2/a2lsp.c \
  $(PROJ_DIR)/Source/Libraries/bv32fp-1.2/allpole.c \
  $(PROJ_DIR)/Source/Libraries/bv32fp-1.2/allzero.c \
  $(PROJ_DIR)/Source/Libraries/bv32fp-1.2/autocor.c \
  $(PROJ_DIR)/Source/Libraries/bv32fp-1.2/bitpack.c \
  $(PROJ_DIR)/Source/Libraries/bv32fp-1.2/bvplc.c \
  $(PROJ_DIR)/Source/Libraries/bv32fp-1.2/cmtables.c \
  $(PROJ_DIR)/Source/Libraries/bv32fp-1.2/coarptch.c \
  $(PROJ_DIR)/Source/Libraries/bv32fp-1.2/decoder.c \
  $(PROJ_DIR)/Source/Libraries/bv32fp-1.2/encoder.c \
  $(PROJ_DIR)/Source/Libraries/bv32fp-1.2/excdec.c \
  $(PROJ_DIR)/Source/Libraries/bv32fp-1.2/excquan.c \
  $(PROJ_DIR)/Source/Libraries/bv32fp-1.2/fineptch.c \
  $(PROJ_DIR)/Source/Libraries/bv32fp-1.2/gaindec.c \
  $(PROJ_DIR)/Source/Libraries/bv32fp-1.2/gainquan.c \
  $(PROJ_DIR)/Source/Libraries/bv32fp-1.2/levdur.c \
  $(PROJ_DIR)/Source/Libraries/bv32fp-1.2/levelest.c \
  $(PROJ_DIR)/Source/Libraries/bv32fp-1.2/lsp2a.c \
  $(PROJ_DIR)/Source/Libraries/bv32fp-1.2/lspdec.c \
  $(PROJ_DIR)/Source/Libraries/bv32fp-1.2/lspquan.c \
  $(PROJ_DIR)/Source/Libraries/bv32fp-1.2/ptdec.c \
  $(PROJ_DIR)/Source/Libraries/bv32fp-1.2/ptquan.c \
  $(PROJ_DIR)/Source/Libraries/bv32fp-1.2/stblchck.c \
  $(PROJ_DIR)/Source/Libraries/bv32fp-1.2/stblzlsp.c \
  $(PROJ_DIR)/Source/Libraries/bv32fp-1.2/tables.c \
  $(PROJ_DIR)/Source/Libraries/bv32fp-1.2/utility.c \
  $(SDK_ROOT)/external/segger_rtt/SEGGER_RTT.c \
  $(SDK_ROOT)/external/segger_rtt/SEGGER_RTT_Syscalls_GCC.c \
  $(SDK_ROOT)/external/segger_rtt/SEGGER_RTT_printf.c \
  $(SDK_ROOT)/components/toolchain/gcc/gcc_startup_nrf52.S \
  $(SDK_ROOT)/components/toolchain/system_nrf52.c \
  $(PROJ_DIR)/Source/Modules/event_bus.c \
  $(PROJ_DIR)/Source/Modules/m_acc.c \
  $(PROJ_DIR)/Source/Modules/m_audio.c \
  $(PROJ_DIR)/Source/Modules/m_audio_frame.c \
  $(PROJ_DIR)/Source/Debug/m_audio_gauges.c \
  $(PROJ_DIR)/Source/Debug/m_audio_probe.c \
  $(PROJ_DIR)/Source/Modules/m_batt_meas.c \
  $(PROJ_DIR)/Source/Modules/m_buzzer.c \
  $(PROJ_DIR)/Source/Modules/m_coms.c \
  $(PROJ_DIR)/Source/Modules/m_coms_ble.c \
  $(PROJ_DIR)/Source/Modules/m_coms_ble_addr.c \
  $(PROJ_DIR)/Source/Modules/m_coms_ble_adv.c \
  $(PROJ_DIR)/Source/Modules/m_coms_ble_atvv.c \
  $(PROJ_DIR)/Source/Modules/m_coms_ble_atvv_srv.c \
  $(PROJ_DIR)/Source/Modules/m_coms_ble_dfu.c \
  $(PROJ_DIR)/Source/Modules/m_coms_ble_hid.c \
  $(PROJ_DIR)/Source/Modules/m_coms_ble_lesc.c \
  $(PROJ_DIR)/Source/Modules/m_gyro.c \
  $(PROJ_DIR)/Source/Modules/m_init.c \
  $(PROJ_DIR)/Source/Modules/m_ir_tx.c \
  $(PROJ_DIR)/Source/Modules/m_keyboard.c \
  $(PROJ_DIR)/Source/Modules/m_leds.c \
  $(PROJ_DIR)/Source/Modules/m_nfc.c \
  $(PROJ_DIR)/Source/Modules/m_protocol_hid.c \
  $(PROJ_DIR)/Source/Modules/m_protocol_hid_state.c \
  $(PROJ_DIR)/Source/Modules/m_protocol_ir.c \
  $(PROJ_DIR)/Source/Modules/m_system_state.c \
  $(PROJ_DIR)/Source/Modules/m_touchpad.c \
  $(SDK_ROOT)/components/ble/common/ble_advdata.c \
  $(SDK_ROOT)/components/ble/common/ble_conn_params.c \
  $(SDK_ROOT)/components/ble/common/ble_conn_state.c \
  $(SDK_ROOT)/components/ble/common/ble_srv_common.c \
  $(SDK_ROOT)/components/ble/peer_manager/gatt_cache_manager.c \
  $(SDK_ROOT)/components/ble/peer_manager/gatts_cache_manager.c \
  $(SDK_ROOT)/components/ble/peer_manager/id_manager.c \
  $(SDK_ROOT)/components/ble/nrf_ble_gatt/nrf_ble_gatt.c \
  $(SDK_ROOT)/components/ble/peer_manager/peer_data_storage.c \
  $(SDK_ROOT)/components/ble/peer_manager/peer_database.c \
  $(SDK_ROOT)/components/ble/peer_manager/peer_id.c \
  $(SDK_ROOT)/components/ble/peer_manager/peer_manager.c \
  $(SDK_ROOT)/components/ble/peer_manager/pm_buffer.c \
  $(SDK_ROOT)/components/ble/peer_manager/pm_mutex.c \
  $(SDK_ROOT)/components/ble/peer_manager/security_dispatcher.c \
  $(SDK_ROOT)/components/ble/peer_manager/security_manager.c \
  $(PROJ_DIR)/Source/Common/app_error_handler.c \
  $(PROJ_DIR)/Source/Common/app_isched.c \
  $(PROJ_DIR)/Source/Common/app_scheduler.c \
  $(PROJ_DIR)/Source/Common/key_combo_util.c \
  $(PROJ_DIR)/Source/Common/rng_monitor.c \
  $(PROJ_DIR)/Source/Common/twi_common.c \
  $(PROJ_DIR)/Source/Debug/app_debug_pin.c \
  $(PROJ_DIR)/Source/Debug/stack_profiler_gcc.s \
  $(SDK_ROOT)/components/nfc/t2t_lib/hal_t2t/hal_nfc_t2t.c \
  $(SDK_ROOT)/components/nfc/ndef/connection_handover/ac_rec/nfc_ac_rec.c \
  $(SDK_ROOT)/components/nfc/ndef/connection_handover/ble_oob_advdata/nfc_ble_oob_advdata.c \
  $(SDK_ROOT)/components/nfc/ndef/connection_handover/common/nfc_ble_pair_common.c \
  $(SDK_ROOT)/components/nfc/ndef/connection_handover/ble_pair_msg/nfc_ble_pair_msg.c \
  $(SDK_ROOT)/components/nfc/ndef/connection_handover/ep_oob_rec/nfc_ep_oob_rec.c \
  $(SDK_ROOT)/components/nfc/ndef/connection_handover/hs_rec/nfc_hs_rec.c \
  $(SDK_ROOT)/components/nfc/ndef/connection_handover/le_oob_rec/nfc_le_oob_rec.c \
  $(SDK_ROOT)/components/nfc/ndef/generic/message/nfc_ndef_msg.c \
  $(SDK_ROOT)/components/nfc/ndef/generic/record/nfc_ndef_record.c \
  $(SDK_ROOT)/components/nfc/ndef/text/nfc_text_rec.c \
  $(SDK_ROOT)/components/softdevice/common/nrf_sdh.c \
  $(SDK_ROOT)/components/softdevice/common/nrf_sdh_ble.c \
  $(SDK_ROOT)/components/softdevice/common/nrf_sdh_soc.c \
  $(SDK_ROOT)/components/libraries/crypto/backend/cc310_lib/cc310_lib_ecdh.c \
  $(SDK_ROOT)/components/libraries/crypto/backend/cc310_lib/cc310_lib_ecdsa.c \
  $(SDK_ROOT)/components/libraries/crypto/backend/cc310_lib/cc310_lib_hash.c \
  $(SDK_ROOT)/components/libraries/crypto/backend/cc310_lib/cc310_lib_init.c \
  $(SDK_ROOT)/components/libraries/crypto/backend/cc310_lib/cc310_lib_keys.c \
  $(SDK_ROOT)/components/libraries/crypto/backend/cc310_lib/cc310_lib_rng.c \
  $(SDK_ROOT)/components/libraries/crypto/backend/cc310_lib/cc310_lib_shared.c \
  $(SDK_ROOT)/components/libraries/crypto/backend/micro_ecc/micro_ecc_lib_ecdh.c \
  $(SDK_ROOT)/components/libraries/crypto/backend/micro_ecc/micro_ecc_lib_ecdsa.c \
  $(SDK_ROOT)/components/libraries/crypto/backend/micro_ecc/micro_ecc_lib_init.c \
  $(SDK_ROOT)/components/libraries/crypto/backend/micro_ecc/micro_ecc_lib_keys.c \
  $(SDK_ROOT)/components/libraries/crypto/backend/micro_ecc/micro_ecc_lib_shared.c \
  $(SDK_ROOT)/components/libraries/crypto/backend/nrf_crypto_sw/nrf_crypto_sw_hash.c \
  $(SDK_ROOT)/components/libraries/crypto/backend/nrf_crypto_sw/nrf_crypto_sw_rng.c \
  $(PROJ_DIR)/Source/Libraries/sbc-0025/srce/sbc_analysis.c \
  $(PROJ_DIR)/Source/Libraries/sbc-0025/srce/sbc_dct.c \
  $(PROJ_DIR)/Source/Libraries/sbc-0025/srce/sbc_dct_coeffs.c \
  $(PROJ_DIR)/Source/Libraries/sbc-0025/srce/sbc_enc_bit_alloc_mono.c \
  $(PROJ_DIR)/Source/Libraries/sbc-0025/srce/sbc_enc_bit_alloc_ste.c \
  $(PROJ_DIR)/Source/Libraries/sbc-0025/srce/sbc_enc_coeffs.c \
  $(PROJ_DIR)/Source/Libraries/sbc-0025/srce/sbc_encoder.c \
  $(PROJ_DIR)/Source/Libraries/sbc-0025/srce/sbc_packing.c \
  $(SDK_ROOT)/components/libraries/experimental_log/src/nrf_log_backend_rtt.c \
  $(SDK_ROOT)/components/libraries/experimental_log/src/nrf_log_backend_serial.c \
  $(SDK_ROOT)/components/libraries/experimental_log/src/nrf_log_backend_uart.c \
  $(SDK_ROOT)/components/libraries/experimental_log/src/nrf_log_frontend.c \
  $(SDK_ROOT)/components/libraries/experimental_log/src/nrf_log_str_formatter.c \
  $(SDK_ROOT)/components/libraries/experimental_mpu/nrf_mpu.c \
  $(SDK_ROOT)/components/libraries/experimental_stack_guard/nrf_stack_guard.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/A2NLSF.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/CNG.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/HP_variable_cutoff.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/LPC_analysis_filter.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/LPC_fit.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/LPC_inv_pred_gain.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/LP_variable_cutoff.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/LTP_analysis_filter_FIX.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/LTP_scale_ctrl_FIX.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/NLSF2A.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/NLSF_VQ.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/NLSF_VQ_weights_laroia.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/NLSF_decode.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/NLSF_del_dec_quant.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/NLSF_encode.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/NLSF_stabilize.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/NLSF_unpack.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/NSQ.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/NSQ_del_dec.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/PLC.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/VAD.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/VQ_WMat_EC.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/ana_filt_bank_1.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/analysis.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/apply_sine_window_FIX.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/autocorr_FIX.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/bands.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/biquad_alt.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/burg_modified_FIX.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/bwexpander.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/bwexpander_32.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/celt.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/celt_decoder.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/celt_encoder.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/celt_lpc.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/arm/celt_pitch_xcorr_arm_gcc.s \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/check_control_input.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/code_signs.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/control_SNR.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/control_audio_bandwidth.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/control_codec.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/corrMatrix_FIX.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/cwrs.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/debug.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/dec_API.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/decode_core.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/decode_frame.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/decode_indices.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/decode_parameters.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/decode_pitch.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/decode_pulses.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/decoder_set_fs.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/enc_API.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/encode_frame_FIX.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/encode_indices.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/encode_pulses.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/entcode.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/entdec.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/entenc.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/find_LPC_FIX.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/find_LTP_FIX.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/find_pitch_lags_FIX.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/find_pred_coefs_FIX.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/gain_quant.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/init_decoder.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/init_encoder.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/inner_prod_aligned.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/interpolate.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/k2a_FIX.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/k2a_Q16_FIX.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/kiss_fft.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/laplace.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/lin2log.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/log2lin.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/mathops.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/mdct.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/mlp.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/mlp_data.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/modes.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/noise_shape_analysis_FIX.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/opus.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/opus_decoder.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/opus_encoder.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/opus_multistream.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/opus_multistream_decoder.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/opus_multistream_encoder.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/pitch.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/pitch_analysis_core_FIX.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/pitch_est_tables.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/process_NLSFs.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/process_gains_FIX.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/quant_LTP_gains.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/quant_bands.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/rate.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/regularize_correlations_FIX.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/repacketizer.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/resampler.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/resampler_down2.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/resampler_down2_3.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/resampler_private_AR2.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/resampler_private_IIR_FIR.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/resampler_private_down_FIR.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/resampler_private_up2_HQ.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/resampler_rom.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/residual_energy16_FIX.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/residual_energy_FIX.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/schur64_FIX.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/schur_FIX.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/shell_coder.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/sigm_Q15.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/sort.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/stereo_LR_to_MS.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/stereo_MS_to_LR.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/stereo_decode_pred.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/stereo_encode_pred.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/stereo_find_predictor.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/stereo_quant_pred.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/sum_sqr_shift.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/table_LSF_cos.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/tables_LTP.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/tables_NLSF_CB_NB_MB.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/tables_NLSF_CB_WB.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/tables_gain.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/tables_other.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/tables_pitch_lag.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/tables_pulses_per_block.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/vector_ops_FIX.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/vq.c \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1/warped_autocorrelation_FIX.c \
  $(SDK_ROOT)/components/libraries/util/app_error.c \
  $(SDK_ROOT)/components/libraries/util/app_error_weak.c \
  $(SDK_ROOT)/components/libraries/gpiote/app_gpiote.c \
  $(SDK_ROOT)/components/libraries/timer/app_timer.c \
  $(SDK_ROOT)/components/libraries/util/app_util_platform.c \
  $(SDK_ROOT)/components/libraries/crc16/crc16.c \
  $(SDK_ROOT)/components/libraries/crc32/crc32.c \
  $(SDK_ROOT)/components/libraries/ecc/ecc.c \
  $(SDK_ROOT)/components/libraries/fds/fds.c \
  $(SDK_ROOT)/components/libraries/hardfault/nrf52/handler/hardfault_handler_gcc.c \
  $(SDK_ROOT)/components/libraries/hardfault/hardfault_implementation.c \
  $(SDK_ROOT)/components/libraries/mem_manager/mem_manager.c \
  $(SDK_ROOT)/components/libraries/util/nrf_assert.c \
  $(SDK_ROOT)/components/libraries/atomic_fifo/nrf_atfifo.c \
  $(SDK_ROOT)/components/libraries/balloc/nrf_balloc.c \
  $(SDK_ROOT)/components/libraries/cli/nrf_cli.c \
  $(SDK_ROOT)/components/libraries/cli/ble_uart/nrf_cli_ble_uart.c \
  $(SDK_ROOT)/components/libraries/cli/rtt/nrf_cli_rtt.c \
  $(SDK_ROOT)/components/libraries/cli/uart/nrf_cli_uart.c \
  $(SDK_ROOT)/external/fprintf/nrf_fprintf.c \
  $(SDK_ROOT)/external/fprintf/nrf_fprintf_format.c \
  $(SDK_ROOT)/components/libraries/fstorage/nrf_fstorage.c \
  $(SDK_ROOT)/components/libraries/fstorage/nrf_fstorage_sd.c \
  $(SDK_ROOT)/components/libraries/experimental_memobj/nrf_memobj.c \
  $(SDK_ROOT)/components/libraries/pwr_mgmt/nrf_pwr_mgmt.c \
  $(SDK_ROOT)/components/libraries/queue/nrf_queue.c \
  $(SDK_ROOT)/components/libraries/experimental_ringbuf/nrf_ringbuf.c \
  $(SDK_ROOT)/components/libraries/experimental_section_vars/nrf_section_iter.c \
  $(SDK_ROOT)/components/libraries/strerror/nrf_strerror.c \
  $(SDK_ROOT)/components/libraries/twi_mngr/nrf_twi_mngr.c \
  $(SDK_ROOT)/components/libraries/util/sdk_mapped_flags.c \
  $(SDK_ROOT)/components/libraries/sha256/sha256.c \
  $(SDK_ROOT)/components/libraries/experimental_task_manager/task_manager.c \
  $(SDK_ROOT)/components/libraries/experimental_task_manager/task_manager_core_armgcc.S \
  $(SDK_ROOT)/components/libraries/crypto/nrf_crypto_ecdh.c \
  $(SDK_ROOT)/components/libraries/crypto/nrf_crypto_ecdsa.c \
  $(SDK_ROOT)/components/libraries/crypto/nrf_crypto_hash.c \
  $(SDK_ROOT)/components/libraries/crypto/nrf_crypto_init.c \
  $(SDK_ROOT)/components/libraries/crypto/nrf_crypto_keys.c \
  $(SDK_ROOT)/components/libraries/crypto/nrf_crypto_mem.c \
  $(SDK_ROOT)/components/libraries/crypto/nrf_crypto_rng.c \

# Include folders common to all targets
INC_FOLDERS += \
  $(PROJ_DIR)/Source/Configuration \
  $(PROJ_DIR)/Source/Libraries \
  $(PROJ_DIR)/Source/Bootloader \
  $(PROJ_DIR)/Source/Debug \
  $(PROJ_DIR)/Source/Modules \
  $(PROJ_DIR)/Source/Libraries/sbc-0025/include \
  $(PROJ_DIR)/Source/Libraries/opus-1.2.1 \
  $(PROJ_DIR)/Source/Common \
  $(PROJ_DIR)/Source/Libraries/bv32fp-1.2 \
  $(PROJ_DIR)/Source/Drivers \
  $(SDK_ROOT)/components/nfc/ndef/generic/message \
  $(SDK_ROOT)/components/nfc/t2t_lib \
  $(SDK_ROOT)/components/drivers_nrf/twi_master \
  $(SDK_ROOT)/components/softdevice/s132/headers/nrf52 \
  $(SDK_ROOT)/components/softdevice/common \
  $(SDK_ROOT)/components/libraries/fstorage \
  $(SDK_ROOT)/components/nfc/ndef/text \
  $(SDK_ROOT)/components/libraries/mutex \
  $(SDK_ROOT)/components/libraries/gpiote \
  $(SDK_ROOT)/components/libraries/experimental_log/src \
  $(SDK_ROOT)/components/drivers_nrf/gpiote \
  $(SDK_ROOT)/components/nfc/ndef/connection_handover/common \
  $(SDK_ROOT)/components/libraries/sha256 \
  $(SDK_ROOT)/components/nfc/ndef/generic/record \
  $(SDK_ROOT)/components/libraries/experimental_memobj \
  $(SDK_ROOT)/components/drivers_nrf/common \
  $(SDK_ROOT)/components/libraries/crypto/backend/micro_ecc \
  $(SDK_ROOT)/components/libraries/experimental_task_manager \
  $(SDK_ROOT)/components/libraries/cli \
  $(SDK_ROOT)/components/nfc/ndef/connection_handover/le_oob_rec \
  $(SDK_ROOT)/components/libraries/queue \
  $(SDK_ROOT)/components/libraries/pwr_mgmt \
  $(SDK_ROOT)/components/libraries/mem_manager \
  $(SDK_ROOT)/components/toolchain/cmsis/include \
  $(SDK_ROOT)/components/drivers_nrf/uart \
  $(SDK_ROOT)/components/ble/common \
  $(SDK_ROOT)/components/drivers_nrf/wdt \
  $(SDK_ROOT)/components/libraries/hardfault/nrf52 \
  $(SDK_ROOT)/components/libraries/cli/uart \
  $(SDK_ROOT)/components/nfc/ndef/connection_handover/ac_rec \
  $(SDK_ROOT)/components/ble/ble_services/ble_bas \
  $(SDK_ROOT)/components/drivers_nrf/ble_flash \
  $(SDK_ROOT)/components/libraries/experimental_section_vars \
  $(SDK_ROOT)/components/softdevice/s132/headers \
  $(SDK_ROOT)/components/libraries/experimental_mpu \
  $(SDK_ROOT)/external/segger_rtt \
  $(SDK_ROOT)/components/drivers_nrf/hal \
  $(SDK_ROOT)/components/ble/ble_services/ble_ias \
  $(SDK_ROOT)/components/drivers_nrf/ppi \
  $(SDK_ROOT)/external/fprintf \
  $(SDK_ROOT)/components/libraries/svc \
  $(SDK_ROOT)/external/nrf_cc310/include \
  $(SDK_ROOT)/components/libraries/experimental_log \
  $(SDK_ROOT)/components/drivers_nrf/delay \
  $(SDK_ROOT)/components/libraries/crc16 \
  $(SDK_ROOT)/components/libraries/util \
  $(SDK_ROOT)/components/drivers_nrf/pwm \
  ../config \
  $(SDK_ROOT)/components/libraries/balloc \
  $(SDK_ROOT)/components/libraries/ecc \
  $(SDK_ROOT)/components/libraries/hardfault \
  $(SDK_ROOT)/components/nfc/ndef/connection_handover/ep_oob_rec \
  $(SDK_ROOT)/components/libraries/timer \
  $(SDK_ROOT)/components/drivers_nrf/rng \
  $(SDK_ROOT)/components/libraries/crypto/backend/nrf_crypto_sw \
  $(SDK_ROOT)/components/libraries/experimental_stack_guard \
  $(SDK_ROOT)/components/toolchain \
  $(SDK_ROOT)/components/libraries/crypto/backend/cc310_lib \
  $(SDK_ROOT)/components/ble/ble_services/ble_nus \
  $(SDK_ROOT)/components/libraries/twi_mngr \
  $(SDK_ROOT)/components/ble/ble_services/ble_hids \
  $(SDK_ROOT)/components/libraries/strerror \
  $(SDK_ROOT)/components/drivers_nrf/pdm \
  $(SDK_ROOT)/components/libraries/crc32 \
  $(SDK_ROOT)/components/nfc/ndef/connection_handover/ble_oob_advdata \
  $(SDK_ROOT)/components/nfc/ndef/connection_handover/ble_pair_msg \
  $(SDK_ROOT)/components/ble/peer_manager \
  $(SDK_ROOT)/external/micro-ecc/micro-ecc \
  $(SDK_ROOT)/components/ble/ble_services/ble_dis \
  $(SDK_ROOT)/components/device \
  $(SDK_ROOT)/components/ble/nrf_ble_gatt \
  $(SDK_ROOT)/components/libraries/cli/rtt \
  $(SDK_ROOT)/components/libraries/cli/ble_uart \
  $(SDK_ROOT)/components/drivers_nrf/saadc \
  $(SDK_ROOT)/components/libraries/atomic_fifo \
  $(SDK_ROOT)/components/libraries/experimental_ringbuf \
  $(SDK_ROOT)/components/libraries/atomic \
  $(SDK_ROOT)/components/libraries/crypto \
  $(SDK_ROOT)/components/toolchain/gcc \
  $(SDK_ROOT)/components/libraries/fds \
  $(SDK_ROOT)/components/drivers_nrf/clock \
  $(SDK_ROOT)/components/nfc/ndef/connection_handover/hs_rec \
  $(SDK_ROOT)/components/nfc/t2t_lib/hal_t2t \

# Libraries common to all targets
LIB_FILES += \
  $(SDK_ROOT)/external/nrf_cc310/lib/libcc310_gcc_0.9.0.a \
  $(PROJ_DIR)/Source/Libraries/vocal_anr_gcc.a \
  $(SDK_ROOT)/external/micro-ecc/nrf52hf_armgcc/armgcc/micro_ecc_lib_nrf52.a \
  $(SDK_ROOT)/components/nfc/t2t_lib/nfc_t2t_lib_gcc.a \

# Optimization flags
OPT = -O2 -g3
# Uncomment the line below to enable link time optimization
#OPT += -flto

# C flags common to all targets
CFLAGS += $(OPT)
CFLAGS += -DAPP_SCHEDULER_ENABLED=1
CFLAGS += -DARM_MATH_CM4
CFLAGS += -DBLE_STACK_SUPPORT_REQD
CFLAGS += -DDISABLE_FLOAT_API
CFLAGS += -DENABLE_ASSERTIONS
CFLAGS += -DFIXED_POINT
CFLAGS += -DFLOAT_ABI_HARD
CFLAGS += -DHAL_NFC_ENGINEERING_BC_FTPAN_WORKAROUND
CFLAGS += -DHAVE_ALLOCA_H
CFLAGS += -DHAVE_LRINT
CFLAGS += -DHAVE_LRINTF
CFLAGS += -DNRF52
CFLAGS += -DNRF52832_XXAA
CFLAGS += -DNRF_SD_BLE_API_VERSION=5
CFLAGS += -DOPUS_ARM_ASM
CFLAGS += -DOPUS_ARM_INLINE_ASM
CFLAGS += -DOPUS_ARM_INLINE_EDSP
CFLAGS += -DOPUS_ARM_INLINE_MEDIA
CFLAGS += -DOPUS_ARM_MAY_HAVE_EDSP
CFLAGS += -DOPUS_ARM_PRESUME_EDSP
CFLAGS += -DOPUS_BUILD
CFLAGS += -DS132
CFLAGS += -DSOFTDEVICE_PRESENT
CFLAGS += -DSWI_DISABLE0
CFLAGS += -DUSE_ALLOCA
CFLAGS += -DUSE_APP_CONFIG
CFLAGS += -D__STARTUP_CONFIG
CFLAGS += -mcpu=cortex-m4
CFLAGS += -mthumb -mabi=aapcs
CFLAGS +=  -Wall -Werror
CFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
# keep every function in a separate section, this allows linker to discard unused ones
CFLAGS += -ffunction-sections -fdata-sections -fno-strict-aliasing
CFLAGS += -fno-builtin -fshort-enums -flto

# C++ flags common to all targets
CXXFLAGS += $(OPT)

# Assembler flags common to all targets
ASMFLAGS += -g3
ASMFLAGS += -mcpu=cortex-m4
ASMFLAGS += -mthumb -mabi=aapcs
ASMFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
ASMFLAGS += -DAPP_SCHEDULER_ENABLED=1
ASMFLAGS += -DARM_MATH_CM4
ASMFLAGS += -DBLE_STACK_SUPPORT_REQD
ASMFLAGS += -DDISABLE_FLOAT_API
ASMFLAGS += -DENABLE_ASSERTIONS
ASMFLAGS += -DFIXED_POINT
ASMFLAGS += -DFLOAT_ABI_HARD
ASMFLAGS += -DHAL_NFC_ENGINEERING_BC_FTPAN_WORKAROUND
ASMFLAGS += -DHAVE_ALLOCA_H
ASMFLAGS += -DHAVE_LRINT
ASMFLAGS += -DHAVE_LRINTF
ASMFLAGS += -DNRF52
ASMFLAGS += -DNRF52832_XXAA
ASMFLAGS += -DNRF_SD_BLE_API_VERSION=5
ASMFLAGS += -DOPUS_ARM_ASM
ASMFLAGS += -DOPUS_ARM_INLINE_ASM
ASMFLAGS += -DOPUS_ARM_INLINE_EDSP
ASMFLAGS += -DOPUS_ARM_INLINE_MEDIA
ASMFLAGS += -DOPUS_ARM_MAY_HAVE_EDSP
ASMFLAGS += -DOPUS_ARM_PRESUME_EDSP
ASMFLAGS += -DOPUS_BUILD
ASMFLAGS += -DS132
ASMFLAGS += -DSOFTDEVICE_PRESENT
ASMFLAGS += -DSWI_DISABLE0
ASMFLAGS += -DUSE_ALLOCA
ASMFLAGS += -DUSE_APP_CONFIG
ASMFLAGS += -D__STARTUP_CONFIG
ASMFLAGS += -flto

# Linker flags
LDFLAGS += $(OPT)
LDFLAGS += -mthumb -mabi=aapcs -L $(TEMPLATE_PATH) -T$(LINKER_SCRIPT)
LDFLAGS += -mcpu=cortex-m4
LDFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
# let linker dump unused sections
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -flto

PCA63519-SR3_nRF52832_Shield: CFLAGS += -DCONFIG_BOARD_NRF52832_PCA63519
PCA63519-SR3_nRF52832_Shield: ASMFLAGS += -DCONFIG_BOARD_NRF52832_PCA63519
CUSTOM-SR3_nRF52832_Custom_Board: CFLAGS += -DCONFIG_BOARD_NRF52832_CUSTOM
CUSTOM-SR3_nRF52832_Custom_Board: ASMFLAGS += -DCONFIG_BOARD_NRF52832_CUSTOM
PCA20023-SR3_nRF52832_Product_Example: CFLAGS += -DCONFIG_BOARD_NRF52832_PCA20023
PCA20023-SR3_nRF52832_Product_Example: ASMFLAGS += -DCONFIG_BOARD_NRF52832_PCA20023

# Add standard libraries at the very end of the linker input, after all objects
# that may need symbols provided by these libraries.
LIB_FILES += -lc -lnosys -lm

.PHONY: default help

# Build all targets by default
default: all

# Print all targets that can be built
help:
	@echo The following targets are available:
	@echo		PCA63519-SR3_nRF52832_Shield
	@echo		CUSTOM-SR3_nRF52832_Custom_Board
	@echo		PCA20023-SR3_nRF52832_Product_Example
	@echo		sdk_config - starting external tool for editing sdk_config.h
	@echo		flash      - flashing binary

TEMPLATE_PATH := $(SDK_ROOT)/components/toolchain/gcc


include $(TEMPLATE_PATH)/Makefile.common

$(foreach target, $(TARGETS), $(call define_target, $(target)))

.PHONY: flash erase

# Flash the program
flash: $(OUTPUT_DIRECTORY)/PCA63519-SR3_nRF52832_Shield.hex
	@echo Flashing: $<
	nrfjprog -f nrf52 --program $< --sectorerase
	nrfjprog -f nrf52 --reset

erase:
	nrfjprog -f nrf52 --eraseall

SDK_CONFIG_FILE := ../config/sdk_config.h
CMSIS_CONFIG_TOOL := $(SDK_ROOT)/external_tools/cmsisconfig/CMSIS_Configuration_Wizard.jar
sdk_config:
	java -jar $(CMSIS_CONFIG_TOOL) $(SDK_CONFIG_FILE)
