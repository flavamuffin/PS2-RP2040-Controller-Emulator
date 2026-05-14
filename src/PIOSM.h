#ifndef PIOSM_h
    #define PIOSM_h

    #if !PICO_NO_HARDWARE
        #include "hardware/pio.h"
    #endif


    // - - - - - frame_keeper - - - - - //

    #define frame_keeper_wrap_target 1
    #define frame_keeper_wrap 7

    static const uint16_t frame_keeper_program_instructions[] =
    {
        0x2092, //  0: wait   1 gpio, 18                 
                //     .wrap_target
        0x2012, //  1: wait   0 gpio, 18                 
        0xc042, //  2: irq    clear 2                    
        0xc000, //  3: irq    nowait 0                   
        0x2092, //  4: wait   1 gpio, 18                 
        0xc043, //  5: irq    clear 3                    
        0xc044, //  6: irq    clear 4                    
        0xc001, //  7: irq    nowait 1                   
                //     .wrap
    };

    #if !PICO_NO_HARDWARE
        static const struct pio_program frame_keeper_program = { .instructions = frame_keeper_program_instructions, .length = 8, .origin = -1, };

        // The configuration that will be applied to the state machine running this program
        static inline pio_sm_config FrameKeeperStateMachineConfig(uint memoryOffset, float clockDivider = 40.0f)
        {
            pio_sm_config stateMachineConfig = pio_get_default_sm_config();
            sm_config_set_wrap(&stateMachineConfig, memoryOffset + frame_keeper_wrap_target, memoryOffset + frame_keeper_wrap);         
            sm_config_set_clkdiv(&stateMachineConfig, clockDivider);    // Sets the frequency that this state machine runs at
            return stateMachineConfig;
        }
    #endif


    // - - - - - time_keeper - - - - - //

    #define time_keeper_wrap_target 0
    #define time_keeper_wrap 2

    static const uint16_t time_keeper_program_instructions[] =
    {
                //     .wrap_target
        0x2014, //  0: wait   0 gpio, 20                 
        0x2094, //  1: wait   1 gpio, 20                 
        0xc043, //  2: irq    clear 3                    
                //     .wrap
    };

    #if !PICO_NO_HARDWARE
        static const struct pio_program time_keeper_program = { .instructions = time_keeper_program_instructions, .length = 3, .origin = -1, };

        // The configuration that will be applied to the state machine running this program
        static inline pio_sm_config TimeKeeperStateMachineConfig(uint memoryOffset, float clockDivider = 40.0f)
        {
            pio_sm_config stateMachineConfig = pio_get_default_sm_config();
            sm_config_set_wrap(&stateMachineConfig, memoryOffset + time_keeper_wrap_target, memoryOffset + time_keeper_wrap); 
            sm_config_set_clkdiv(&stateMachineConfig, clockDivider);    // Sets the frequency that this state machine runs at
            return stateMachineConfig;
        }
    #endif


    // - - - - - interface_rx - - - - - //

    #define interface_rx_wrap_target 2
    #define interface_rx_wrap 4

    static const uint16_t interface_rx_program_instructions[] =
    {
        0xa0c3, //  0: mov    isr, null                  
        0xc022, //  1: irq    wait 2                     
                //     .wrap_target
        0xc023, //  2: irq    wait 3                     
        0x00c0, //  3: jmp    pin, 0                     
        0x4001, //  4: in     pins, 1                    
                //     .wrap
    };

    #if !PICO_NO_HARDWARE
        static const struct pio_program interface_rx_program = { .instructions = interface_rx_program_instructions, .length = 5, .origin = -1, };

        // The configuration that will be applied to the state machine running this program
        static inline pio_sm_config RXStateMachineConfig(uint memoryOffset, uint8_t commandPin, uint8_t attentionPin, float clockDivider = 40.0f)
        {
            pio_sm_config stateMachineConfig = pio_get_default_sm_config();
            sm_config_set_wrap(&stateMachineConfig, memoryOffset + interface_rx_wrap_target, memoryOffset + interface_rx_wrap);

            sm_config_set_in_shift(&stateMachineConfig, true, true, 8);     // Shift right for LSB transmissions, Autopush once the ISR has a byte in it
            sm_config_set_in_pins(&stateMachineConfig, commandPin);         // The command pin is used to read input from the PlayStation
            sm_config_set_jmp_pin(&stateMachineConfig, attentionPin);       // The attention pin is used to detect the end of the frame, and causes a jump
            
            sm_config_set_clkdiv(&stateMachineConfig, clockDivider);        // Sets the frequency that this state machine runs at

            return stateMachineConfig;
        }
    #endif





    // - - - - - interface_tx - - - - - //

    #define interface_tx_wrap_target 0
    #define interface_tx_wrap 10

    static const uint16_t interface_tx_program_instructions[] =
    {
                //     .wrap_target
        0x80a0, //  0: pull   block                      
        0xe346, //  1: set    y, 6                   [3] 
        0x0782, //  2: jmp    y--, 2                 [7] 
        0xfb42, //  3: set    y, 2            side 1 [3] 
        0x0784, //  4: jmp    y--, 4                 [7] 
        0xf047, //  5: set    y, 7            side 0     
        0x2014, //  6: wait   0 gpio, 20                 
        0x6081, //  7: out    pindirs, 1                 
        0x2094, //  8: wait   1 gpio, 20                 
        0x0086, //  9: jmp    y--, 6                     
        0xe080, // 10: set    pindirs, 0                 
                //     .wrap
    };

    #if !PICO_NO_HARDWARE
        static const struct pio_program interface_tx_program = { .instructions = interface_tx_program_instructions, .length = 11, .origin = -1, };

        static inline pio_sm_config interface_tx_program_get_default_config(uint offset)
        {
            pio_sm_config stateMachineConfig = pio_get_default_sm_config();
            sm_config_set_wrap(&stateMachineConfig, offset + interface_tx_wrap_target, offset + interface_tx_wrap);
            return stateMachineConfig;
        }

        // The configuration that will be applied to the state machine running this program
        static inline pio_sm_config TXStateMachineConfig(uint memoryOffset, uint8_t acknowledgePin, uint8_t attentionPin, uint8_t dataPin, float clockDivider = 40.0f)
        {
            pio_sm_config stateMachineConfig = pio_get_default_sm_config();
            sm_config_set_wrap(&stateMachineConfig, memoryOffset + interface_tx_wrap_target, memoryOffset + interface_tx_wrap);

            sm_config_set_out_shift(&stateMachineConfig, true, false, 32);      // Shift right for LSB transmissions, No autopush
            sm_config_set_out_pins(&stateMachineConfig, dataPin, 1);            // The data pin is used to read input from the PlayStation
            sm_config_set_set_pins(&stateMachineConfig, dataPin, 2); 
            sm_config_set_sideset_pins(&stateMachineConfig, acknowledgePin);
            sm_config_set_sideset(&stateMachineConfig, 2, true, true);
            sm_config_set_jmp_pin(&stateMachineConfig, attentionPin);           // The attention pin is used to detect the end of the frame, and causes a jump

            sm_config_set_clkdiv(&stateMachineConfig, clockDivider);            // Sets the frequency that this state machine runs at

            return stateMachineConfig;
        }
    #endif

#endif