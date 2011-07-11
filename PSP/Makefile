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
		
clean:
	make cleankernelprx
	make cleanexceptionprx
	make cleanlaunchereboot
	make cleanengineprx
