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
		control-gpios = <&gpio0 5 GPIO_ACTIVE_HIGH>;
	};
};
```

Now, update your `shield.keymap` adding the behaviors.

```keymap

/* enums to set in sources to filter events in zmk,output-behavior-listener */
#define OUTPUT_SOURCE_LAYER_STATE_CHANGE        1
#define OUTPUT_SOURCE_POSITION_STATE_CHANGE     2
#define OUTPUT_SOURCE_KEYCODE_STATE_CHANGE      3
#define OUTPUT_SOURCE_MOUSE_BUTTON_STATE_CHANGE 4
#define OUTPUT_SOURCE_TRANSPORT                 5

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
                compatible = "zmk,output-behavior-generic";
                #binding-cells = <0>;
                device = <&lra0>;
                delay = <1>;
                time-to-live-ms = <30>;
                /* reserved, not-implement-yet */
                force = <80>;
        };
        ob_lar0_out: ob_generic_lar0_out {
                compatible = "zmk,output-behavior-generic";
                #binding-cells = <0>;
                device = <&lra0>;
                delay = <133>;
                time-to-live-ms = <10>;
                /* reserved, not-implement-yet */
                force = <30>;
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
