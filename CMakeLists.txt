zephyr_library()

if ((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

  zephyr_library_sources_ifdef(CONFIG_ZMK_OUTPUT_SPLIT_OUTOUT_RELAY src/output_split_output_relay.c)
  zephyr_library_sources_ifdef(CONFIG_ZMK_OUTPUT_BEHAVIOR_GENERIC src/output_behavior_generic.c)
  zephyr_library_sources_ifdef(CONFIG_ZMK_OUTPUT_BEHAVIOR_LISTENER src/output_behavior_listener.c)

endif()

# if (CONFIG_ZMK_SPLIT AND (NOT CONFIG_ZMK_SPLIT_ROLE_CENTRAL))
# endif()

zephyr_library_sources_ifdef(CONFIG_ZMK_OUTPUT_GENERIC src/output_generic.c)
zephyr_library_sources_ifdef(CONFIG_ZMK_OUTPUT_HAPTIC_FEEDBACK src/output_haptic_feedback.c)
zephyr_library_sources_ifdef(CONFIG_ZMK_OUTPUT_MOTORIZED_FADER src/output_motorized_fader.c)
zephyr_library_sources_ifdef(CONFIG_ZMK_OUTPUT_PWM src/output_pwm.c)

zephyr_library_sources_ifdef(CONFIG_ZMK_OUTPUT_BEHAVIOR_LISTENER src/output_event.c)
zephyr_include_directories(include)

zephyr_include_directories(${APPLICATION_SOURCE_DIR}/include)
