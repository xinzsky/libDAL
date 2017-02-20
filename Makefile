
INSTALL_PATH = /usr/local
BIN_INSTALL_PATH = $(INSTALL_PATH)/bin
LIB_INSTALL_PATH = $(INSTALL_PATH)/lib
INC_INSTALL_PATH = $(INSTALL_PATH)/include

INC_PATH = -I $(INC_INSTALL_PATH)
LIB_PATH = 
LIBS = -ldal
LIBS2 = 

CC = gcc
CXX = g++
MTFlAGS = -D_REENTRANT
MACROS = 
NDBG = -DNDEBUG
SOFLAGS = -fPIC
CFLAGS = -Wall $(MACROS) $(MTFlAGS) $(INC_PATH)
CXXFLAGS = -Wall $(MACROS) $(MTFlAGS) $(INC_PATH)
RM = rm -f
AR = ar
ARFLAGS = rcv
MAKE = gmake

LINKERNAME = libdal.so
SONAME = $(LINKERNAME).1
LIBNAME = $(LINKERNAME).1.0.0

LIB_TARGET = $(LIBNAME) 
BIN_TARGET = 
TARGET = $(LIB_TARGET) $(BIN_TARGET)

LIB_OBJS = sharding.o loadbalance.o dal.o conf.o xmalloc.o xstring.o
BIN_OBJS =
OBJS = $(LIB_OBJS) $(BIN_OBJS)
INCS = sharding.h loadbalance.h dal.h conf.h xmalloc.h xstring.h

all: $(TARGET)
.PHONE:all

$(LIBNAME): CFLAGS += $(SOFLAGS)
$(LIBNAME): $(LIB_OBJS)
	$(CC) -shared -Wl,-soname,$(SONAME) -o $@ $^

debug: CFLAGS += -g
debug: CXXFLAGS += -g
debug: all

install:
	cp -f  --target-directory=$(INC_INSTALL_PATH) $(INCS)
	cp -f $(LIB_TARGET) $(LIB_INSTALL_PATH)
	ln -fs $(LIB_TARGET)  $(LIB_INSTALL_PATH)/$(LINKERNAME)

clean:
	$(RM) $(TARGET) $(OBJS)

test_dal:
	gcc -Wall -g -o test_dal -DTEST_DAL dal.c $(LIBS) $(LIBS2) $(INC_PATH)

