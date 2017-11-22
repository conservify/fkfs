BUILD=build

default: all

$(BUILD):
	mkdir -p $(BUILD)

all: $(BUILD)
	cd $(BUILD) && cmake ../
	cd $(BUILD) && make
