TEMPLATE = none

include ../build/main.mk

LIBS   = ${ROOT}/out/cmsis/libcmsis.a \
		 ${ROOT}/out/libc/libc.a \
		 ${ROOT}/out/freertos/libfreertos.a \
		 ${ROOT}/out/application/libapplication.a
		 
IMAGES = can-flash.hex can-flash can-ram

all: $(addprefix ${BUILD}/,${IMAGES})

${BUILD}/%: %.lds ${OBJECTS} ${LIBS}
	@mkdir -p $(dir $@)
	${LD} -o $@ ${LDFLAGS} -\( -T$^ -\)

${BUILD}/%.hex: ${BUILD}/%
	${OBJCOPY} -O ihex $< $@

clean:
	rm -rf ${BUILD}

run-%: %.gdb ${BUILD}/% 
	${GDB} --batch -x $^

flash-%: ${BUILD}/%-flash.hex
	gdbflasher --mcu stm32f10xx_md $<

debug-%: %.gdb ${BUILD}/% 
	${GDB} -x $^
