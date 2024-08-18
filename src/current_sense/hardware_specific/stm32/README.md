This is a new unified current sense driver for STM32. It replaces the per STM Family/board drivers.

### Supported Families
It supports the following families:
- F1
- F4
- F7
- G4
- L4

If the family is not supported, it will:
- use analogread for Inline current sense
- fail for Low-side current 

The driver was mostly written in a STM family agnotic way, but it wasn't tested on other STM families.<BR>
There are few things to be checked when extending it:
- STM families (C0,F0,G0,L0 and WL) don't have Injected ADC so Regular ADC should be used. But DMA with interrupt is not yet implemented which prevents the use with a timer without repetition counter and Lowside current sense
- DMA can have different implementation for each STM family (Instance/channel/stream/request). This information is only available in the datasheet or in STM32CUBEMX
- For the ADC interrupts, there can be different IRQs and IRQ Handlers
- Some STM families can only use one common sampling time on same ADC (F0 and L0)
- Some STM families can only use 2 common sampling time on same ADC (C0,G0,U5,WBA and WL)

### ADCs
The driver can use several ADCs in parallel (if available).<BR>
You can check for your variant in [STM32Duino](https://github.com/stm32duino/Arduino_Core_STM32/blob/b24801b4b473649fb6d5bc51c22a69a64d45b732/variants/STM32F1xx/F103R(C-D-E)T/PeripheralPins.c#L38) if a pin can be sampled by several ADCs (e.g. PA_0 is sampled on ADC1, PA_0_ALT2 is sampled on ADC3).<BR>
This can help reduce the sampling time.

### Injected ADC
Injected ADC is used for sampling that requires a critical timing (e.g. low side current sensing).<BR>
The driver picks some of the shortest sampling time available on your STM32 chip.<BR>
You can force a specific sampling time with the ADC_SAMPLINGTIME_INJ build flag (e.g. ADC_SAMPLINGTIME_INJ=ADC_SAMPLETIME_1CYCLE_5).<BR>
Up to 4 injected channels can be sampled per ADC, you will get an error message beyond that.<BR>

### Regular ADC
Regular ADC is used for less critical sampling.<BR>
It uses DMA to copy ADC samples to a buffer.<BR>
If not triggered by a timer, it will sample in continuous mode.<BR>
You can force a specific sampling time with the ADC_SAMPLINGTIME build flag (e.g. ADC_SAMPLINGTIME=ADC_SAMPLETIME_8CYCLES_5).<BR>
Up to 16 inected channels can be sampled per ADC, you will get an error message beyond that.<BR>
If many ADCs are available, this can lead to a lot of RAM usage. You can use the MAX_REG_ADC_CHANNELS build flag to reduce this. (e.g. MAX_REG_ADC_CHANNELS=4).<BR>

### Internal channels
You can now sample internal channels (VREF,VBAT,TEMP).<BR>
Those channels require a long sampling time, so the driver picks some of the longest sampling times available on your STM32 chip.<BR>
You can force a specific sampling time with the ADC_SAMPLINGTIME_INTERNAL build flag (e.g. ADC_SAMPLINGTIME_INTERNAL=ADC_SAMPLETIME_480CYCLES).<BR>

### Interrupt
An interrupt is used with low side current sensing if the timer used to trigger the ADC has no repetition counter, which is used to make sure the low side mosfets are ON.<BR>
You can force the use of an interrupt if needed with the SIMPLEFOC_STM32_ADC_INTERRUPT build flag.

