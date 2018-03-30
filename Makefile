BUILD=build

default: all testing

$(BUILD):
	mkdir -p $(BUILD)

all: $(BUILD) gitdeps build/fkfs-read-darwin-amd64 build/fkfs-read-linux-amd64
	cd $(BUILD) && cmake ../ && make

test: all
	cd $(BUILD) && make test

build/fkfs-read-darwin-amd64: src/*.go
	env GOOS=darwin GOARCH=amd64 go build -o $@ $^

build/fkfs-read-linux-amd64: src/*.go
	env GOOS=linux GOARCH=amd64 go build -o $@ $^

gitdeps:
	simple-deps --config examples/simple/arduino-libraries
	simple-deps --config examples/dma/arduino-libraries

testing: .PHONY
	mkdir -p testing/build
	cd testing/build && cmake ../
	cd testing/build && make

clean:
	rm -rf testing/build
	rm -rf $(BUILD)

veryclean: clean
	rm -rf gitdeps

.PHONY:
