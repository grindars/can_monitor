CPPFLAGS += -Iinclude

all: ${BUILD}/lib${TARGET}.a

${BUILD}/lib${TARGET}.a: $(OBJECTS)
	${AR} rcs $@ $^

clean:
	rm -rf ${BUILD}
