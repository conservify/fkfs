BUILD=build

default: all

$(BUILD):
	mkdir -p $(BUILD)

all: $(BUILD) gitdeps
	cd $(BUILD) && cmake ../
	cd $(BUILD) && make

gitdeps:
	simple-deps --config examples/simple/arduino-libraries
	simple-deps --config examples/dma/arduino-libraries

clean:
	rm -rf $(BUILD)

veryclean: clean
	rm -rf gitdeps
