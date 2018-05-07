GOARCH ?= amd64
GOOS ?= linux
GO ?= env GOOS=$(GOOS) GOARCH=$(GOARCH) go
BUILD ?= build
BUILDARCH ?= $(BUILD)/$(GOOS)-$(GOARCH)

default: all testing test

$(BUILD):
	mkdir -p $(BUILD)

$(BUILDARCH):
	mkdir -p $(BUILDARCH)

all: $(BUILD) gitdeps
	env GOOS=darwin GOARCH=amd64 make binaries-all
	env GOOS=linux GOARCH=amd64 make binaries-all
	cd $(BUILD) && cmake ../ && make

install:
	env GOOS=darwin GOARCH=amd64 make binaries-install
	env GOOS=linux GOARCH=amd64 make binaries-install

test: all
	cd $(BUILD) && make test

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

binaries-install: binaries-all
	cp $(BUILDARCH)/fkfs-read $(INSTALLDIR)

binaries-all: $(BUILDARCH) $(BUILDARCH)/fkfs-read

$(BUILDARCH)/fkfs-read: src/*.go
	go build -o $@ $^

.PHONY: $(BUILD)
