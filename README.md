# Output Behavior Listener

This module add behaviors to output device for ZMK.

## What it does

It allow config a feedback of state change event by binding behaviors to feedback devices, such as, Linear Resonant Actuator (LRA) vibration motors, or LED indicators. It is made for simulating various sensations by vibrating in a designed sequence.

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
    ...
```

Now, update your `board.overlay` adding the necessary bits (update the pins for your board accordingly):

```dts
/ {
        /* setup wired gpio from actual device. */
        /*   e.g. a Linear Resonant Actuator (LRA) vibration motor, or a LED */
	lra0: output_generic_0 {
		compatible = "zmk,output-generic";
		#binding-cells = <0>;
		control-gpios = <&gpio0 4 GPIO_ACTIVE_HIGH>;
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
        lar0_obl {
                compatible = "zmk,output-behavior-listener";

                /* only enable in layer(s) */
                layers = <DEFAULT RAISE>;

                /* only enable for these subscribed stated event(s) */
                sources = <
                        OUTPUT_SOURCE_LAYER_STATE_CHANGE
                        OUTPUT_SOURCE_KEYCODE_STATE_CHANGE
                        >;

                /* setup output behavior to output two phase vibration or LED animation */
                bindings = < &ob_lar0_in &ob_lar0_out >;
        };

        /* setup TWO phases of vibration output on same LRA device (<&lra0>) */
        /* phase 1: -(delay 1ms)---(vibrate 30ms) */
        /* phase 2: -(delay 133ms)-----------------------(vibrate 10ms) */
        ob_lar0_in: ob_generic_lar0_in {
                compatible = "zmk,output-behavior-generic"; #binding-cells = <0>;
                device = <&lra0>; delay = <1>; time-to-live-ms = <30>;
        };
        ob_lar0_out: ob_generic_lar0_out {
                compatible = "zmk,output-behavior-generic"; #binding-cells = <0>;
                device = <&lra0>; delay = <133>; time-to-live-ms = <10>;
        };

        /*** ...and, more user cases on below... ***/

        /* setup listener on enter raise layer */
        lar0_obl__enter_raise_layer {
                compatible = "zmk,output-behavior-listener";
                layers = < RAISE >;
                sources = < OUTPUT_SOURCE_LAYER_STATE_CHANGE >;
                bindings = < &ob_lar0_in &ob_lar0_out >;
        };

        /* setup listener on back to default layer from another layer */
        lar0_obl__back_to_default_layer {
                compatible = "zmk,output-behavior-listener";
                layers = < DEFAULT >;
                sources = < OUTPUT_SOURCE_LAYER_STATE_CHANGE >;
                
                /* optional, to catch the state becomes negative boolean on state_changed */
                invert-state;
                
                /* optional, specify a layer which is leaving from */
                position = < RAISE >;

                bindings = < &ob_lar0_out >;
        };

        /* setup listener on press key on position 0 */
        lar0_obl__press_position_0 {
                compatible = "zmk,output-behavior-listener";
                layers = < DEFAULT >;
                sources = < OUTPUT_SOURCE_POSITION_STATE_CHANGE >;
                /* optional, set position of switch to bind to */
                position = <0>;
                bindings = < &ob_lar0_in >;
        };

        /* setup listener on press keycode 'D' */
        lar0_obl__press_key_code_D {
                compatible = "zmk,output-behavior-listener";
                layers = < DEFAULT >;
                sources = < OUTPUT_SOURCE_KEYCODE_STATE_CHANGE >;
                /* optional, set keycode filter here */
                position = < 0x07 >;
                bindings = < &ob_lar0_in >;
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
