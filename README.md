# ZMK Output Behavior Listener

This module add behaviors to output device for ZMK.

## What it does

It allows to config a feedback of state change event by binding behaviors to feedback devices, such as, eccentric rotating mass (ERM) motors, Linear Resonant Actuator (LRA) vibration motors, LED indicators, serve motors, etc. It is made for simulating various feedback effect pattern in a designed sequence on devices simultaneously.

There is an extension module [zmk-split-peripheral-output-relay](https://github.com/badjeff/zmk-split-peripheral-output-relay) that proxies output to split peripherals  via bluetooth.

To drive LRA, it is recommended to drive it with [DRV2605L Haptic Motor Controller](https://www.adafruit.com/product/2305) and [zmk-drv2605-driver](https://github.com/badjeff/zmk-drv2605-driver).

## Installation

Include this project on your ZMK's west manifest in `config/west.yml`:

```yaml
manifest:
  ...
  projects:
    ...
    - name: zmk-output-behavior-listener
      remote: badjeff
      revision: main
    - name: zmk-drv2605-driver
      remote: badjeff
      revision: main
    ...
```

Now, update your `board.overlay` adding the necessary bits (update the pins for your board accordingly):

```dts
/ {
        /* setup wired gpio from actual device. */
        /*   e.g. a eccentric rotating mass (ERM) motor, or a LED */
	erm0: output_generic_0 {
		compatible = "zmk,output-generic";
		#binding-cells = <0>;
		control-gpios = <&gpio0 4 GPIO_ACTIVE_HIGH>;
	};

        /* assign feedback device from actual DRV2605 i2c device */
	lra0: output_haptic_fb_0 {
		compatible = "zmk,output-haptic-feedback";
		#binding-cells = <0>;
                /* only one driver type is supported now */
		driver = "drv2605";
                /* labled i2c device with [zmk-drv2605-driver] */
		device = <&drv2605_0>;
	};

        /* setup wired PWM device to control a LED */
	pwm_led0: output_pwm_0 {
		compatible = "zmk,output-pwm";
		#binding-cells = <0>;
		pwms = <&pwm0 0 PWM_MSEC(20) PWM_POLARITY_NORMAL>;
	};
};
```

Now, update your `shield.keymap` adding the behaviors.

```keymap

/* enums to set in sources to filter events in zmk,output-behavior-listener */
#define OUTPUT_SOURCE_LAYER_STATE_CHANGE        1
#define OUTPUT_SOURCE_POSITION_STATE_CHANGE     2
#define OUTPUT_SOURCE_KEYCODE_STATE_CHANGE      3
#define OUTPUT_SOURCE_TRANSPORT                 4

/ {
        /* setup listen to monitor layer_state_change and keycode_state_change */
        erm0_obl {
                compatible = "zmk,output-behavior-listener";

                /* only enable in layer(s) */
                layers = <DEFAULT RAISE>;

                /* only enable for these subscribed stated event(s) */
                sources = <
                        OUTPUT_SOURCE_LAYER_STATE_CHANGE
                        OUTPUT_SOURCE_KEYCODE_STATE_CHANGE
                        >;

                /* setup output behavior to output two phase vibration or LED animation */
                bindings = < &ob_erm0_in &ob_erm0_out >;
        };

        /* setup TWO phases of vibration output on same ERM device (<&erm0>) */
        /* phase 1: -(delay 1ms)---(vibrate 30ms) */
        /* phase 2: -(delay 133ms)-----------------------(vibrate 10ms) */
        ob_erm0_in: ob_generic_erm0_in {
                compatible = "zmk,output-behavior-generic"; #binding-cells = <0>;
                device = <&erm0>; delay = <1>; time-to-live-ms = <30>;
        };
        ob_erm0_out: ob_generic_erm0_out {
                compatible = "zmk,output-behavior-generic"; #binding-cells = <0>;
                device = <&erm0>; delay = <133>; time-to-live-ms = <10>;
        };

        /* setup behavior for another LRA device, which is droven by DRV2605 module (<&lra0>) */
        ob_lra0: ob_generic_lra0_in {
                compatible = "zmk,output-behavior-generic"; #binding-cells = <0>;
                device = <&lra0>;

                /* force will be convrt to waveformm effect from DRV2605 library */
                /* NOTE: <7> is Soft Bump at 100% */
                force = <7>;
        };

        ob_lra0: ob_generic_lra0_in {
                compatible = "zmk,output-pwm"; #binding-cells = <0>;
                device = <&pwm_led0>;

                /* force will be convrt to waveformm effect from DRV2605 library */
                /* NOTE: <7> is Soft Bump at 100% */
                force = <7>;
        };

        /* setup behavior for PWM device */
        ob_pwm0: ob_generic_pwn0 {
                compatible = "zmk,output-behavior-generic"; #binding-cells = <0>;
                device = <&pwm_led0>;
                
                /* set duty cycle of pwm, max 256 */
                /* pwm duty cycle raise to <180> on key press */
                force = <180>;

                /* enable momentum to trigger on both on all state change */
                momentum;

                /* pwm duty cycle drop to <12> on key release */
                momentum-force = <12>;
        };

        /*** ...and, more user cases on below... ***/

        /* setup listener on enter raise layer */
        erm0_obl__enter_raise_layer {
                compatible = "zmk,output-behavior-listener";
                layers = < RAISE >;
                sources = < OUTPUT_SOURCE_LAYER_STATE_CHANGE >;
                bindings = < &ob_erm0_in &ob_erm0_out >;
        };

        /* setup listener on back to default layer from another layer */
        erm0_obl__back_to_default_layer {
                compatible = "zmk,output-behavior-listener";
                layers = < DEFAULT >;
                sources = < OUTPUT_SOURCE_LAYER_STATE_CHANGE >;
                
                /* optional, to catch the state becomes negative boolean on state_changed */
                invert-state;
                
                /* optional, specify a layer which is leaving from */
                position = < RAISE >;

                bindings = < &ob_erm0_out >;
        };

        /* setup listener on press key on position 0 */
        erm0_obl__press_position_0 {
                compatible = "zmk,output-behavior-listener";
                layers = < DEFAULT >;
                sources = < OUTPUT_SOURCE_POSITION_STATE_CHANGE >;

                /* set position of switch to bind to */
                position = <0>;
                bindings = < &ob_erm0_in >;
        };

        /* setup listener on press keycode 'D' */
        erm0_obl__press_key_code_D {
                compatible = "zmk,output-behavior-listener";
                layers = < DEFAULT >;
                sources = < OUTPUT_SOURCE_KEYCODE_STATE_CHANGE >;

                /* set keycode filter here */
                position = < 0x07 >;
                bindings = < &ob_erm0_in >;
        };

        /* setup listener on press keycode 'Q', and feedback via LRA */
        erm0_obl__press_key_code_Q {
                compatible = "zmk,output-behavior-listener";
                layers = < DEFAULT >;
                sources = < OUTPUT_SOURCE_KEYCODE_STATE_CHANGE >;

                /* set keycode filter here */
                position = < 0x14 >;
                bindings = < &ob_lra0 >;
                
                /* enable to catch all state change that include key press and release */
                /* ensure to stop on-going LRA effect immediately on key released */
                all-state;
        };

        /* setup listener on press keycode 'Q', and feedback via LRA */
        pwm0_obl__press_key_code_P {
                compatible = "zmk,output-behavior-listener";
                layers = < DEFAULT >;
                sources = < OUTPUT_SOURCE_KEYCODE_STATE_CHANGE >;

                /* set keycode filter here */
                position = < 0x13 >;
                bindings = < &ob_pwm0 >;
                
                /* enable to catch all state change that include key press and release */
                /* ensure to stop on-going LRA effect immediately on key released */
                all-state;
        };

        keymap {
                compatible = "zmk,keymap";
                default_layer {
                        bindings = <
                                ... &mo RAISE ...
                        >;
                };
                raise_layer {
                        bindings = <
                                ... &trans ...
                        >;
                };
       };
};
```
