################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
USB_API/USB_Common/dma.obj: ../USB_API/USB_Common/dma.c $(GEN_OPTS) $(GEN_SRCS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv5/tools/compiler/msp430/bin/cl430" -vmspx --abi=coffabi -g --include_path="C:/ti/ccsv5/ccs_base/msp430/include" --include_path="C:/AndyOct2012/CodeComposerWorkspaceV5_1/USB3pt2DaysimeterIthaca" --include_path="C:/AndyOct2012/CodeComposerWorkspaceV5_1/USB3pt2DaysimeterIthaca/F5xx_F6xx_Core_Lib" --include_path="C:/AndyOct2012/CodeComposerWorkspaceV5_1/USB3pt2DaysimeterIthaca/USB_API" --include_path="C:/AndyOct2012/CodeComposerWorkspaceV5_1/USB3pt2DaysimeterIthaca/USB_config" --include_path="C:/ti/ccsv5/tools/compiler/msp430/include" --define=__MSP430F5528__ --diag_warning=225 --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU23 --silicon_errata=CPU40 --printf_support=minimal --preproc_with_compile --preproc_dependency="USB_API/USB_Common/dma.pp" --obj_directory="USB_API/USB_Common" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

USB_API/USB_Common/usb.obj: ../USB_API/USB_Common/usb.c $(GEN_OPTS) $(GEN_SRCS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv5/tools/compiler/msp430/bin/cl430" -vmspx --abi=coffabi -g --include_path="C:/ti/ccsv5/ccs_base/msp430/include" --include_path="C:/AndyOct2012/CodeComposerWorkspaceV5_1/USB3pt2DaysimeterIthaca" --include_path="C:/AndyOct2012/CodeComposerWorkspaceV5_1/USB3pt2DaysimeterIthaca/F5xx_F6xx_Core_Lib" --include_path="C:/AndyOct2012/CodeComposerWorkspaceV5_1/USB3pt2DaysimeterIthaca/USB_API" --include_path="C:/AndyOct2012/CodeComposerWorkspaceV5_1/USB3pt2DaysimeterIthaca/USB_config" --include_path="C:/ti/ccsv5/tools/compiler/msp430/include" --define=__MSP430F5528__ --diag_warning=225 --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU23 --silicon_errata=CPU40 --printf_support=minimal --preproc_with_compile --preproc_dependency="USB_API/USB_Common/usb.pp" --obj_directory="USB_API/USB_Common" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


