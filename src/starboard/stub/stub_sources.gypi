# Copyright 2018 The Cobalt Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
{
  'variables': {
    'stub_sources': [
        '<(DEPTH)/starboard/shared/starboard/application.cc',
        '<(DEPTH)/starboard/shared/starboard/command_line.cc',
        '<(DEPTH)/starboard/shared/starboard/command_line.h',
        '<(DEPTH)/starboard/shared/starboard/event_cancel.cc',
        '<(DEPTH)/starboard/shared/starboard/event_schedule.cc',
        '<(DEPTH)/starboard/shared/starboard/file_mode_string_to_flags.cc',
        '<(DEPTH)/starboard/shared/starboard/log_message.cc',
        '<(DEPTH)/starboard/shared/starboard/player/filter/stub_player_components_impl.cc',
        '<(DEPTH)/starboard/shared/starboard/queue_application.cc',
        '<(DEPTH)/starboard/shared/stub/accessibility_get_caption_settings.cc',
        '<(DEPTH)/starboard/shared/stub/accessibility_get_display_settings.cc',
        '<(DEPTH)/starboard/shared/stub/accessibility_get_text_to_speech_settings.cc',
        '<(DEPTH)/starboard/shared/stub/accessibility_set_captions_enabled.cc',
        '<(DEPTH)/starboard/shared/stub/atomic_public.h',
        '<(DEPTH)/starboard/shared/stub/audio_sink_create.cc',
        '<(DEPTH)/starboard/shared/stub/audio_sink_destroy.cc',
        '<(DEPTH)/starboard/shared/stub/audio_sink_get_max_channels.cc',
        '<(DEPTH)/starboard/shared/stub/audio_sink_get_nearest_supported_sample_frequency.cc',
        '<(DEPTH)/starboard/shared/stub/audio_sink_is_audio_frame_storage_type_supported.cc',
        '<(DEPTH)/starboard/shared/stub/audio_sink_is_audio_sample_type_supported.cc',
        '<(DEPTH)/starboard/shared/stub/audio_sink_is_valid.cc',
        '<(DEPTH)/starboard/shared/stub/byte_swap.cc',
        '<(DEPTH)/starboard/shared/stub/character_is_alphanumeric.cc',
        '<(DEPTH)/starboard/shared/stub/character_is_digit.cc',
        '<(DEPTH)/starboard/shared/stub/character_is_hex_digit.cc',
        '<(DEPTH)/starboard/shared/stub/character_is_space.cc',
        '<(DEPTH)/starboard/shared/stub/character_is_upper.cc',
        '<(DEPTH)/starboard/shared/stub/character_to_lower.cc',
        '<(DEPTH)/starboard/shared/stub/character_to_upper.cc',
        '<(DEPTH)/starboard/shared/stub/condition_variable_broadcast.cc',
        '<(DEPTH)/starboard/shared/stub/condition_variable_create.cc',
        '<(DEPTH)/starboard/shared/stub/condition_variable_destroy.cc',
        '<(DEPTH)/starboard/shared/stub/condition_variable_signal.cc',
        '<(DEPTH)/starboard/shared/stub/condition_variable_wait.cc',
        '<(DEPTH)/starboard/shared/stub/condition_variable_wait_timed.cc',
        '<(DEPTH)/starboard/shared/stub/cryptography_create_transformer.cc',
        '<(DEPTH)/starboard/shared/stub/cryptography_destroy_transformer.cc',
        '<(DEPTH)/starboard/shared/stub/cryptography_get_tag.cc',
        '<(DEPTH)/starboard/shared/stub/cryptography_set_authenticated_data.cc',
        '<(DEPTH)/starboard/shared/stub/cryptography_set_initialization_vector.cc',
        '<(DEPTH)/starboard/shared/stub/cryptography_transform.cc',
        '<(DEPTH)/starboard/shared/stub/directory_can_open.cc',
        '<(DEPTH)/starboard/shared/stub/directory_close.cc',
        '<(DEPTH)/starboard/shared/stub/directory_create.cc',
        '<(DEPTH)/starboard/shared/stub/directory_get_next.cc',
        '<(DEPTH)/starboard/shared/stub/directory_open.cc',
        '<(DEPTH)/starboard/shared/stub/double_absolute.cc',
        '<(DEPTH)/starboard/shared/stub/double_exponent.cc',
        '<(DEPTH)/starboard/shared/stub/double_floor.cc',
        '<(DEPTH)/starboard/shared/stub/double_is_finite.cc',
        '<(DEPTH)/starboard/shared/stub/double_is_nan.cc',
        '<(DEPTH)/starboard/shared/stub/drm_close_session.cc',
        '<(DEPTH)/starboard/shared/stub/drm_create_system.cc',
        '<(DEPTH)/starboard/shared/stub/drm_destroy_system.cc',
        '<(DEPTH)/starboard/shared/stub/drm_generate_session_update_request.cc',
        '<(DEPTH)/starboard/shared/stub/drm_is_server_certificate_updatable.cc',
        '<(DEPTH)/starboard/shared/stub/drm_system_internal.h',
        '<(DEPTH)/starboard/shared/stub/drm_update_server_certificate.cc',
        '<(DEPTH)/starboard/shared/stub/drm_update_session.cc',
        '<(DEPTH)/starboard/shared/stub/file_can_open.cc',
        '<(DEPTH)/starboard/shared/stub/file_close.cc',
        '<(DEPTH)/starboard/shared/stub/file_delete.cc',
        '<(DEPTH)/starboard/shared/stub/file_exists.cc',
        '<(DEPTH)/starboard/shared/stub/file_flush.cc',
        '<(DEPTH)/starboard/shared/stub/file_get_info.cc',
        '<(DEPTH)/starboard/shared/stub/file_get_path_info.cc',
        '<(DEPTH)/starboard/shared/stub/file_open.cc',
        '<(DEPTH)/starboard/shared/stub/file_read.cc',
        '<(DEPTH)/starboard/shared/stub/file_seek.cc',
        '<(DEPTH)/starboard/shared/stub/file_truncate.cc',
        '<(DEPTH)/starboard/shared/stub/file_write.cc',
        '<(DEPTH)/starboard/shared/stub/log.cc',
        '<(DEPTH)/starboard/shared/stub/log_flush.cc',
        '<(DEPTH)/starboard/shared/stub/log_format.cc',
        '<(DEPTH)/starboard/shared/stub/log_is_tty.cc',
        '<(DEPTH)/starboard/shared/stub/log_raw.cc',
        '<(DEPTH)/starboard/shared/stub/log_raw_dump_stack.cc',
        '<(DEPTH)/starboard/shared/stub/log_raw_format.cc',
        '<(DEPTH)/starboard/shared/stub/media_can_play_mime_and_key_system.cc',
        '<(DEPTH)/starboard/shared/stub/media_get_audio_buffer_budget.cc',
        '<(DEPTH)/starboard/shared/stub/media_get_audio_configuration.cc',
        '<(DEPTH)/starboard/shared/stub/media_get_audio_output_count.cc',
        '<(DEPTH)/starboard/shared/stub/media_get_buffer_alignment.cc',
        '<(DEPTH)/starboard/shared/stub/media_get_buffer_allocation_unit.cc',
        '<(DEPTH)/starboard/shared/stub/media_get_buffer_garbage_collection_duration_threshold.cc',
        '<(DEPTH)/starboard/shared/stub/media_get_buffer_padding.cc',
        '<(DEPTH)/starboard/shared/stub/media_get_buffer_storage_type.cc',
        '<(DEPTH)/starboard/shared/stub/media_get_initial_buffer_capacity.cc',
        '<(DEPTH)/starboard/shared/stub/media_get_max_buffer_capacity.cc',
        '<(DEPTH)/starboard/shared/stub/media_get_progressive_buffer_budget.cc',
        '<(DEPTH)/starboard/shared/stub/media_get_video_buffer_budget.cc',
        '<(DEPTH)/starboard/shared/stub/media_is_audio_supported.cc',
        '<(DEPTH)/starboard/shared/stub/media_is_buffer_pool_allocate_on_demand.cc',
        '<(DEPTH)/starboard/shared/stub/media_is_buffer_using_memory_pool.cc',
        '<(DEPTH)/starboard/shared/stub/media_is_output_protected.cc',
        '<(DEPTH)/starboard/shared/stub/media_is_supported.cc',
        '<(DEPTH)/starboard/shared/stub/media_is_transfer_characteristics_supported.cc',
        '<(DEPTH)/starboard/shared/stub/media_is_video_supported.cc',
        '<(DEPTH)/starboard/shared/stub/media_set_output_protection.cc',
        '<(DEPTH)/starboard/shared/stub/memory_allocate_aligned_unchecked.cc',
        '<(DEPTH)/starboard/shared/stub/memory_allocate_unchecked.cc',
        '<(DEPTH)/starboard/shared/stub/memory_compare.cc',
        '<(DEPTH)/starboard/shared/stub/memory_copy.cc',
        '<(DEPTH)/starboard/shared/stub/memory_find_byte.cc',
        '<(DEPTH)/starboard/shared/stub/memory_flush.cc',
        '<(DEPTH)/starboard/shared/stub/memory_free.cc',
        '<(DEPTH)/starboard/shared/stub/memory_free_aligned.cc',
        '<(DEPTH)/starboard/shared/stub/memory_get_stack_bounds.cc',
        '<(DEPTH)/starboard/shared/stub/memory_map.cc',
        '<(DEPTH)/starboard/shared/stub/memory_move.cc',
        '<(DEPTH)/starboard/shared/stub/memory_protect.cc',
        '<(DEPTH)/starboard/shared/stub/memory_reallocate_unchecked.cc',
        '<(DEPTH)/starboard/shared/stub/memory_set.cc',
        '<(DEPTH)/starboard/shared/stub/memory_unmap.cc',
        '<(DEPTH)/starboard/shared/stub/microphone_close.cc',
        '<(DEPTH)/starboard/shared/stub/microphone_create.cc',
        '<(DEPTH)/starboard/shared/stub/microphone_destroy.cc',
        '<(DEPTH)/starboard/shared/stub/microphone_get_available.cc',
        '<(DEPTH)/starboard/shared/stub/microphone_is_sample_rate_supported.cc',
        '<(DEPTH)/starboard/shared/stub/microphone_open.cc',
        '<(DEPTH)/starboard/shared/stub/microphone_read.cc',
        '<(DEPTH)/starboard/shared/stub/mutex_acquire.cc',
        '<(DEPTH)/starboard/shared/stub/mutex_acquire_try.cc',
        '<(DEPTH)/starboard/shared/stub/mutex_create.cc',
        '<(DEPTH)/starboard/shared/stub/mutex_destroy.cc',
        '<(DEPTH)/starboard/shared/stub/mutex_release.cc',
        '<(DEPTH)/starboard/shared/stub/once.cc',
        '<(DEPTH)/starboard/shared/stub/player_create.cc',
        '<(DEPTH)/starboard/shared/stub/player_destroy.cc',
        '<(DEPTH)/starboard/shared/stub/player_get_current_frame.cc',
        '<(DEPTH)/starboard/shared/stub/player_get_info.cc',
        '<(DEPTH)/starboard/shared/stub/player_get_info2.cc',
        '<(DEPTH)/starboard/shared/stub/player_get_maximum_number_of_samples_per_write.cc',
        '<(DEPTH)/starboard/shared/stub/player_output_mode_supported.cc',
        '<(DEPTH)/starboard/shared/stub/player_seek.cc',
        '<(DEPTH)/starboard/shared/stub/player_seek2.cc',
        '<(DEPTH)/starboard/shared/stub/player_set_bounds.cc',
        '<(DEPTH)/starboard/shared/stub/player_set_playback_rate.cc',
        '<(DEPTH)/starboard/shared/stub/player_set_volume.cc',
        '<(DEPTH)/starboard/shared/stub/player_write_end_of_stream.cc',
        '<(DEPTH)/starboard/shared/stub/player_write_sample.cc',
        '<(DEPTH)/starboard/shared/stub/player_write_sample2.cc',
        '<(DEPTH)/starboard/shared/stub/socket_accept.cc',
        '<(DEPTH)/starboard/shared/stub/socket_bind.cc',
        '<(DEPTH)/starboard/shared/stub/socket_clear_last_error.cc',
        '<(DEPTH)/starboard/shared/stub/socket_connect.cc',
        '<(DEPTH)/starboard/shared/stub/socket_create.cc',
        '<(DEPTH)/starboard/shared/stub/socket_destroy.cc',
        '<(DEPTH)/starboard/shared/stub/socket_free_resolution.cc',
        '<(DEPTH)/starboard/shared/stub/socket_get_interface_address.cc',
        '<(DEPTH)/starboard/shared/stub/socket_get_last_error.cc',
        '<(DEPTH)/starboard/shared/stub/socket_get_local_address.cc',
        '<(DEPTH)/starboard/shared/stub/socket_is_connected.cc',
        '<(DEPTH)/starboard/shared/stub/socket_is_connected_and_idle.cc',
        '<(DEPTH)/starboard/shared/stub/socket_join_multicast_group.cc',
        '<(DEPTH)/starboard/shared/stub/socket_listen.cc',
        '<(DEPTH)/starboard/shared/stub/socket_receive_from.cc',
        '<(DEPTH)/starboard/shared/stub/socket_resolve.cc',
        '<(DEPTH)/starboard/shared/stub/socket_send_to.cc',
        '<(DEPTH)/starboard/shared/stub/socket_set_broadcast.cc',
        '<(DEPTH)/starboard/shared/stub/socket_set_receive_buffer_size.cc',
        '<(DEPTH)/starboard/shared/stub/socket_set_reuse_address.cc',
        '<(DEPTH)/starboard/shared/stub/socket_set_send_buffer_size.cc',
        '<(DEPTH)/starboard/shared/stub/socket_set_tcp_keep_alive.cc',
        '<(DEPTH)/starboard/shared/stub/socket_set_tcp_no_delay.cc',
        '<(DEPTH)/starboard/shared/stub/socket_set_tcp_window_scaling.cc',
        '<(DEPTH)/starboard/shared/stub/socket_waiter_add.cc',
        '<(DEPTH)/starboard/shared/stub/socket_waiter_create.cc',
        '<(DEPTH)/starboard/shared/stub/socket_waiter_destroy.cc',
        '<(DEPTH)/starboard/shared/stub/socket_waiter_remove.cc',
        '<(DEPTH)/starboard/shared/stub/socket_waiter_wait.cc',
        '<(DEPTH)/starboard/shared/stub/socket_waiter_wait_timed.cc',
        '<(DEPTH)/starboard/shared/stub/socket_waiter_wake_up.cc',
        '<(DEPTH)/starboard/shared/stub/speech_recognizer_cancel.cc',
        '<(DEPTH)/starboard/shared/stub/speech_recognizer_create.cc',
        '<(DEPTH)/starboard/shared/stub/speech_recognizer_destroy.cc',
        '<(DEPTH)/starboard/shared/stub/speech_recognizer_start.cc',
        '<(DEPTH)/starboard/shared/stub/speech_recognizer_stop.cc',
        '<(DEPTH)/starboard/shared/stub/speech_synthesis_cancel.cc',
        '<(DEPTH)/starboard/shared/stub/speech_synthesis_speak.cc',
        '<(DEPTH)/starboard/shared/stub/storage_close_record.cc',
        '<(DEPTH)/starboard/shared/stub/storage_delete_record.cc',
        '<(DEPTH)/starboard/shared/stub/storage_get_record_size.cc',
        '<(DEPTH)/starboard/shared/stub/storage_open_record.cc',
        '<(DEPTH)/starboard/shared/stub/storage_read_record.cc',
        '<(DEPTH)/starboard/shared/stub/storage_write_record.cc',
        '<(DEPTH)/starboard/shared/stub/string_compare.cc',
        '<(DEPTH)/starboard/shared/stub/string_compare_all.cc',
        '<(DEPTH)/starboard/shared/stub/string_compare_no_case.cc',
        '<(DEPTH)/starboard/shared/stub/string_compare_no_case_n.cc',
        '<(DEPTH)/starboard/shared/stub/string_compare_wide.cc',
        '<(DEPTH)/starboard/shared/stub/string_concat.cc',
        '<(DEPTH)/starboard/shared/stub/string_concat_wide.cc',
        '<(DEPTH)/starboard/shared/stub/string_copy.cc',
        '<(DEPTH)/starboard/shared/stub/string_copy_wide.cc',
        '<(DEPTH)/starboard/shared/stub/string_duplicate.cc',
        '<(DEPTH)/starboard/shared/stub/string_find_character.cc',
        '<(DEPTH)/starboard/shared/stub/string_find_last_character.cc',
        '<(DEPTH)/starboard/shared/stub/string_find_string.cc',
        '<(DEPTH)/starboard/shared/stub/string_format.cc',
        '<(DEPTH)/starboard/shared/stub/string_format_wide.cc',
        '<(DEPTH)/starboard/shared/stub/string_get_length.cc',
        '<(DEPTH)/starboard/shared/stub/string_get_length_wide.cc',
        '<(DEPTH)/starboard/shared/stub/string_parse_double.cc',
        '<(DEPTH)/starboard/shared/stub/string_parse_signed_integer.cc',
        '<(DEPTH)/starboard/shared/stub/string_parse_uint64.cc',
        '<(DEPTH)/starboard/shared/stub/string_parse_unsigned_integer.cc',
        '<(DEPTH)/starboard/shared/stub/string_scan.cc',
        '<(DEPTH)/starboard/shared/stub/system_binary_search.cc',
        '<(DEPTH)/starboard/shared/stub/system_break_into_debugger.cc',
        '<(DEPTH)/starboard/shared/stub/system_clear_last_error.cc',
        '<(DEPTH)/starboard/shared/stub/system_get_connection_type.cc',
        '<(DEPTH)/starboard/shared/stub/system_get_device_type.cc',
        '<(DEPTH)/starboard/shared/stub/system_get_error_string.cc',
        '<(DEPTH)/starboard/shared/stub/system_get_extensions.cc',
        '<(DEPTH)/starboard/shared/stub/system_get_last_error.cc',
        '<(DEPTH)/starboard/shared/stub/system_get_locale_id.cc',
        '<(DEPTH)/starboard/shared/stub/system_get_number_of_processors.cc',
        '<(DEPTH)/starboard/shared/stub/system_get_path.cc',
        '<(DEPTH)/starboard/shared/stub/system_get_property.cc',
        '<(DEPTH)/starboard/shared/stub/system_get_random_data.cc',
        '<(DEPTH)/starboard/shared/stub/system_get_random_uint64.cc',
        '<(DEPTH)/starboard/shared/stub/system_get_stack.cc',
        '<(DEPTH)/starboard/shared/stub/system_get_total_cpu_memory.cc',
        '<(DEPTH)/starboard/shared/stub/system_get_total_gpu_memory.cc',
        '<(DEPTH)/starboard/shared/stub/system_get_used_cpu_memory.cc',
        '<(DEPTH)/starboard/shared/stub/system_get_used_gpu_memory.cc',
        '<(DEPTH)/starboard/shared/stub/system_has_capability.cc',
        '<(DEPTH)/starboard/shared/stub/system_hide_splash_screen.cc',
        '<(DEPTH)/starboard/shared/stub/system_is_debugger_attached.cc',
        '<(DEPTH)/starboard/shared/stub/system_raise_platform_error.cc',
        '<(DEPTH)/starboard/shared/stub/system_request_pause.cc',
        '<(DEPTH)/starboard/shared/stub/system_request_stop.cc',
        '<(DEPTH)/starboard/shared/stub/system_request_suspend.cc',
        '<(DEPTH)/starboard/shared/stub/system_request_unpause.cc',
        '<(DEPTH)/starboard/shared/stub/system_sort.cc',
        '<(DEPTH)/starboard/shared/stub/system_supports_resume.cc',
        '<(DEPTH)/starboard/shared/stub/system_symbolize.cc',
        '<(DEPTH)/starboard/shared/stub/thread_context_get_pointer.cc',
        '<(DEPTH)/starboard/shared/stub/thread_create.cc',
        '<(DEPTH)/starboard/shared/stub/thread_create_local_key.cc',
        '<(DEPTH)/starboard/shared/stub/thread_destroy_local_key.cc',
        '<(DEPTH)/starboard/shared/stub/thread_detach.cc',
        '<(DEPTH)/starboard/shared/stub/thread_get_current.cc',
        '<(DEPTH)/starboard/shared/stub/thread_get_id.cc',
        '<(DEPTH)/starboard/shared/stub/thread_get_local_value.cc',
        '<(DEPTH)/starboard/shared/stub/thread_get_name.cc',
        '<(DEPTH)/starboard/shared/stub/thread_is_equal.cc',
        '<(DEPTH)/starboard/shared/stub/thread_join.cc',
        '<(DEPTH)/starboard/shared/stub/thread_sampler_create.cc',
        '<(DEPTH)/starboard/shared/stub/thread_sampler_destroy.cc',
        '<(DEPTH)/starboard/shared/stub/thread_sampler_freeze.cc',
        '<(DEPTH)/starboard/shared/stub/thread_sampler_is_supported.cc',
        '<(DEPTH)/starboard/shared/stub/thread_sampler_thaw.cc',
        '<(DEPTH)/starboard/shared/stub/thread_set_local_value.cc',
        '<(DEPTH)/starboard/shared/stub/thread_set_name.cc',
        '<(DEPTH)/starboard/shared/stub/thread_sleep.cc',
        '<(DEPTH)/starboard/shared/stub/thread_types_public.h',
        '<(DEPTH)/starboard/shared/stub/thread_yield.cc',
        '<(DEPTH)/starboard/shared/stub/time_get_monotonic_now.cc',
        '<(DEPTH)/starboard/shared/stub/time_get_monotonic_thread_now.cc',
        '<(DEPTH)/starboard/shared/stub/time_get_now.cc',
        '<(DEPTH)/starboard/shared/stub/time_zone_get_current.cc',
        '<(DEPTH)/starboard/shared/stub/time_zone_get_dst_name.cc',
        '<(DEPTH)/starboard/shared/stub/time_zone_get_name.cc',
        '<(DEPTH)/starboard/shared/stub/user_get_current.cc',
        '<(DEPTH)/starboard/shared/stub/user_get_property.cc',
        '<(DEPTH)/starboard/shared/stub/user_get_signed_in.cc',
        '<(DEPTH)/starboard/shared/stub/window_create.cc',
        '<(DEPTH)/starboard/shared/stub/window_destroy.cc',
        '<(DEPTH)/starboard/shared/stub/window_get_diagonal_size_in_inches.cc',
        '<(DEPTH)/starboard/shared/stub/window_get_platform_handle.cc',
        '<(DEPTH)/starboard/shared/stub/window_get_size.cc',
        '<(DEPTH)/starboard/shared/stub/window_on_screen_keyboard_suggestions_supported.cc',
        '<(DEPTH)/starboard/shared/stub/window_set_default_options.cc',
        '<(DEPTH)/starboard/shared/stub/window_update_on_screen_keyboard_suggestions.cc',
    ],
  },
}
