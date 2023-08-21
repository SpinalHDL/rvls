CXX      := -c++
CXXFLAGS := -pedantic-errors -Wall -Wextra -Werror
LDFLAGS  := -L/usr/lib -lstdc++ -lm
BUILD    := ./build
OBJ_DIR  := $(BUILD)/objects
APP_DIR  := $(BUILD)/apps
TARGET   := rvls

SRC := $(wildcard src/*.cpp)

SPIKE?=../riscv-isa-sim
SPIKE_BUILD=$(realpath ${SPIKE}/build)
SPIKE_OBJS:= libspike_main.a  libriscv.a  libdisasm.a  libsoftfloat.a  libfesvr.a  libfdt.a
SPIKE_OBJS:=$(addprefix ${SPIKE_BUILD}/,${SPIKE_OBJS})
LDFLAGS+=${SPIKE_OBJS}
LDFLAGS += -L/usr/lib/x86_64-linux-gnu
LIBRARIES += -lpthread -ldl -lboost_regex -lboost_system -lpthread  -lboost_system -lboost_regex 

INCLUDE += -I$(realpath ${SPIKE}/riscv)
INCLUDE += -I$(realpath ${SPIKE}/fesvr)
INCLUDE += -I$(realpath ${SPIKE}/softfloat)
INCLUDE += -I$(realpath ${SPIKE_BUILD})


JNI_INCLUDE ?= $(dir $(shell readlink -f $(shell which javac)))../include

ifneq ($(JNI_INCLUDE),)
ifneq ($(JNI_INCLUDE),../include)
	CXXFLAGS += -I${JNI_INCLUDE}
	CXXFLAGS += -I${JNI_INCLUDE}/linux
	CXXFLAGS += -DRVLS_JNI
endif
endif


CXXFLAGS += -Wno-unused-parameter -Wno-pedantic -Wno-unused-variable -Wno-unused-function
CXXFLAGS += -O0 -g3
LDFLAGS += -O0 -g3



OBJECTS  := $(SRC:%.cpp=$(OBJ_DIR)/%.o)
DEPENDENCIES \
         := $(OBJECTS:.o=.d)

all: build $(APP_DIR)/$(TARGET)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) -fPIC -shared $(CXXFLAGS) $(INCLUDE) -c $< -MMD -o $@

$(APP_DIR)/$(TARGET).so: $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) -fPIC -shared $(CXXFLAGS) -o $(APP_DIR)/$(TARGET).so $^ $(LDFLAGS) $(LIBRARIES)
	

$(APP_DIR)/$(TARGET): $(APP_DIR)/$(TARGET).so
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $(APP_DIR)/$(TARGET) $^ $(LDFLAGS) $(LIBRARIES)
	

-include $(DEPENDENCIES)

.PHONY: all build clean debug release info

build:
	@mkdir -p $(APP_DIR)
	@mkdir -p $(OBJ_DIR)

debug: CXXFLAGS += -DDEBUG -g
debug: all

release: CXXFLAGS += 
release: all

clean:
	-@rm -rvf $(OBJ_DIR)/*
	-@rm -rvf $(APP_DIR)/*

info:
	@echo "[*] Application dir: ${APP_DIR}     "
	@echo "[*] Object dir:      ${OBJ_DIR}     "
	@echo "[*] Sources:         ${SRC}         "
	@echo "[*] Objects:         ${OBJECTS}     "
	@echo "[*] Dependencies:    ${DEPENDENCIES}"
                       