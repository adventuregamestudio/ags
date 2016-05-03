DISTRIB_NAME?=AGS_Runtime_for_PSP_3.21_R9
PSP_AGS_FOLDER?=j:/psp/game/ags
COPY?=0

export DISTRIB_NAME
export PSP_AGS_FOLDER
export COPY

DIST_PREFIX=../PSP/dist/$(DISTRIB_NAME)

.PHONY: kernelprx exceptionprx launchereboot engineprx allplugins \
        cleankernelprx cleanexceptionprx cleanlaunchereboot cleanengineprx cleanallplugins \
        distrib

all: kernelprx exceptionprx launchereboot engineprx allplugins

kernelprx:
	@$(MAKE) -C "kernel"
	@cp "kernel/kernel.prx" "bin"
ifeq ($(COPY), 1)
	@cp "kernel/kernel.prx" $(PSP_AGS_FOLDER)
endif

exceptionprx:
	@$(MAKE) -C "exception/prx"
	@cp "exception/prx/exception.prx" "bin"
ifeq ($(COPY), 1)
	@cp "exception/prx/exception.prx" $(PSP_AGS_FOLDER)
endif

launchereboot:
	@$(MAKE) -C "launcher"
	@cp "launcher/EBOOT.PBP" "bin"
ifeq ($(COPY), 1)
	@cp "launcher/EBOOT.PBP" $(PSP_AGS_FOLDER)
endif

engineprx:
	@$(MAKE) -C "../Engine" -f Makefile.psp
	@cp "../Engine/ags321.prx" "bin"
ifeq ($(COPY), 1)
	@cp "../Engine/ags321.prx" $(PSP_AGS_FOLDER)
endif

allplugins:
	@$(MAKE) -C "../Plugins" -f Makefile.psp

cleankernelprx:
	@$(MAKE) -C "kernel" clean
	@rm -f "bin/kernel.prx"

cleanexceptionprx:
	@$(MAKE) -C "exception/prx" clean
	@rm -f "bin/exception.prx"

cleanlaunchereboot:
	@$(MAKE) -C "launcher" clean
	@rm -f "bin/EBOOT.PBP"

cleanengineprx:
	@$(MAKE) C "../Engine" -f Makefile.psp clean
	@rm -f "bin/ags321.prx"

cleanallplugins:
	@$(MAKE) -C "../Plugins" -f Makefile.psp clean

distrib: all
	@mkdir -p "$(DIST_PREFIX)"
	@cp "bin/EBOOT.PBP" "$(DIST_PREFIX)/EBOOT.PBP"
	@cp "bin/ags321.prx" "$(DIST_PREFIX)/ags321.prx"
	@cp "bin/kernel.prx" "$(DIST_PREFIX)/kernel.prx"
	@cp "bin/exception.prx" "$(DIST_PREFIX)/exception.prx"
	@cp "bin/psp.cfg" "$(DIST_PREFIX)/psp.cfg"
	@cp "README.md" "$(DIST_PREFIX)/readme.txt"
	@cp "../License.txt" "$(DIST_PREFIX)/License.txt"
	@cp "../Copyright.txt" "$(DIST_PREFIX)/Copyright.txt"
	@$(MAKE) -C "../Plugins" -f Makefile.psp distrib

clean: cleankernelprx cleanexceptionprx cleanlaunchereboot cleanengineprx cleanallplugins
