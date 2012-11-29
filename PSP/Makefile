DISTRIB_NAME = AGS_Runtime_for_PSP_3.21_R9
PSP_AGS_FOLDER = "j:/psp/game/ags"

.PHONY: kernelprx exceptionprx launchereboot engineprx allplugins \
        cleankernelprx cleanexceptionprx cleanlaunchereboot cleanengineprx cleanallplugins \
        distrib

all:
	make kernelprx
	make exceptionprx
	make launchereboot
	make engineprx
	make allplugins

kernelprx:
	@make -C "kernel"
	@cp "kernel/kernel.prx" "bin"
ifeq ($(COPY), 1)
	@cp "kernel/kernel.prx" $(PSP_AGS_FOLDER)
endif

exceptionprx:
	@make -C "exception/prx"
	@cp "exception/prx/exception.prx" "bin"
ifeq ($(COPY), 1)
	@cp "exception/prx/exception.prx" $(PSP_AGS_FOLDER)
endif

launchereboot:
	@make -C "launcher"
	@cp "launcher/EBOOT.PBP" "bin"
ifeq ($(COPY), 1)
	@cp "launcher/EBOOT.PBP" $(PSP_AGS_FOLDER)
endif

engineprx:
	@make -C "../Engine" -f Makefile.psp
	@cp "../Engine/ags321.prx" "bin"
ifeq ($(COPY), 1)
	@cp "../Engine/ags321.prx" $(PSP_AGS_FOLDER)
endif

allplugins:
ifeq ($(COPY), 1)
	@make -C "../Plugins" -f Makefile.psp COPY=1
else
	@make -C "../Plugins" -f Makefile.psp
endif
	

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
	@make clean -C "../Engine" -f Makefile.psp
	@rm -f "bin/ags321.prx"

cleanallplugins:
	@make clean -C "../Plugins" -f Makefile.psp

distrib: all
	@mkdir "dist" || true
	@mkdir "dist/$(DISTRIB_NAME)" || true
	@cp "bin/EBOOT.PBP" "dist/$(DISTRIB_NAME)/EBOOT.PBP"
	@cp "bin/ags321.prx" "dist/$(DISTRIB_NAME)/ags321.prx"
	@cp "bin/kernel.prx" "dist/$(DISTRIB_NAME)/kernel.prx"
	@cp "bin/exception.prx" "dist/$(DISTRIB_NAME)/exception.prx"
	@cp "bin/psp.cfg" "dist/$(DISTRIB_NAME)/psp.cfg"
	@cp "readme.md" "dist/$(DISTRIB_NAME)/readme.txt"
	@cp "../License.txt" "dist/$(DISTRIB_NAME)/License.txt"
	@cp "../Copyright.txt" "dist/$(DISTRIB_NAME)/License.txt"
	@make distrib -C "../Plugins" -f Makefile.psp

clean:
	make cleankernelprx
	make cleanexceptionprx
	make cleanlaunchereboot
	make cleanengineprx
	make cleanallplugins
