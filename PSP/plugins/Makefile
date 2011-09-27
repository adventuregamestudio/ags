DISTRIB_NAME = AGS_Runtime_for_PSP_3.21_R6
PSP_AGS_FOLDER = "j:/psp/game/ags"

all:
	make blend
	make snowrain
	make flashlight

blend:
	@make -C "agsblend"
	@cp "agsblend/agsblend.prx" "../bin"
ifeq ($(COPY), 1)
	@cp "agsblend/agsblend.prx" $(PSP_AGS_FOLDER)
endif

snowrain:
	@make -C "ags_snowrain"
	@cp "ags_snowrain/ags_snowrain.prx" "../bin"
ifeq ($(COPY), 1)
	@cp "ags_snowrain/ags_snowrain.prx" $(PSP_AGS_FOLDER)
endif

flashlight:
	@make -C "agsflashlight"
	@cp "agsflashlight/agsflashlight.prx" "../bin"
ifeq ($(COPY), 1)
	@cp "agsflashlight/agsflashlight.prx" $(PSP_AGS_FOLDER)
endif

cleanblend:
	@make clean -C "agsblend"
	@rm -f "../bin/agsblend.prx"
	
cleansnowrain:
	@make clean -C "ags_snowrain"
	@rm -f "../bin/ags_snowrain.prx"

cleanflashlight:
	@make clean -C "agsflashlight"
	@rm -f "../bin/agsflashlight.prx"

distrib:
	@mkdir "../dist" || true
	@mkdir "../dist/$(DISTRIB_NAME)" || true
	@cp "../bin/ags_snowrain.prx" "../dist/$(DISTRIB_NAME)/ags_snowrain.prx"
	@cp "../bin/agsblend.prx" "../dist/$(DISTRIB_NAME)/agsblend.prx"
	@cp "../bin/agsflashlight.prx" "../dist/$(DISTRIB_NAME)/agsflashlight.prx"
	@cp "agsblend/License.txt" "../dist/$(DISTRIB_NAME)/License_AGSBlend.txt"

clean:
	make cleanblend
	make cleansnowrain
	make cleanflashlight
