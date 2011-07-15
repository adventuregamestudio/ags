DISTRIB_NAME = AGS_Runtime_for_PSP_3.21

all:
	make kernelprx
	make exceptionprx
	make launchereboot
	make engineprx

kernelprx:
	@make -C "kernel"
	@cp "kernel/kernel.prx" "bin"

exceptionprx:
	@make -C "exception/prx"
	@cp "exception/prx/exception.prx" "bin"

launchereboot:
	@make -C "launcher"
	@cp "launcher/EBOOT.PBP" "bin"

engineprx:
	@make -C "../Engine"
	@cp "../Engine/ags321.prx" "bin"

cleankernelprx:
	@make clean -C "kernel"
	@rm -f "bin/kernel.prx"
	
cleanexceptionprx:
	@make clean -C "exception/prx"
	@rm -f "bin/exception.prx"

cleanlaunchereboot:
	@make clean -C "launcher"
	@rm -f "bin/EBOOT.PBP"

cleanengineprx:
	@make clean -C "../Engine"
	@rm -f "bin/ags321.prx"

distrib:
	@mkdir "dist" || true
	@mkdir "dist/$(DISTRIB_NAME)" || true
	@cp "bin/EBOOT.PBP" "dist/$(DISTRIB_NAME)/EBOOT.PBP"
	@cp "bin/ags321.prx" "dist/$(DISTRIB_NAME)/ags321.prx"
	@cp "bin/kernel.prx" "dist/$(DISTRIB_NAME)/kernel.prx"
	@cp "bin/exception.prx" "dist/$(DISTRIB_NAME)/exception.prx"
	@cp "bin/psp.cfg" "dist/$(DISTRIB_NAME)/psp.cfg"
	@cp "readme.txt" "dist/$(DISTRIB_NAME)/readme.txt"
	@cp "../Engine/License.txt" "dist/$(DISTRIB_NAME)/License.txt"

clean:
	make cleankernelprx
	make cleanexceptionprx
	make cleanlaunchereboot
	make cleanengineprx
