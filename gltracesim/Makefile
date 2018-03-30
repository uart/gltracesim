PIN_HOME:=ext/pin-2.14-71313-gcc.4.4.7-linux
MESA_HOME:=ext/mesa
PROTO_HOME:=ext/protobuf/install
PROTOC:=$(PROTO_HOME)/bin/protoc

_CXXFLAGS=-std=c++11 -DBIGARRAY_MULTIPLIER=1 -Wall -Werror -g \
 -Wno-unknown-pragmas -fno-stack-protector -DTARGET_IA32E \
 -DHOST_IA32E -fPIC -DTARGET_LINUX -O3 -fomit-frame-pointer \
 -fno-strict-aliasing \
 -I$(PIN_HOME)/source/include/pin \
 -I$(PIN_HOME)/source/include/pin/gen \
 -I$(PIN_HOME)/extras/components/include \
 -I$(PIN_HOME)/extras/xed-intel64/include \
 -I$(MESA_HOME)/include/ \
 -I$(MESA_HOME)/src/ \
 -I$(MESA_HOME)/src/gallium/include/ \
 -I$(MESA_HOME)/src/gallium/auxiliary/ \
 -I$(PROTO_HOME)/include/ \
 -Isrc/ -I. $(CXXFLAGS)


_LDFLAGS=-shared -Wl,--hash-style=sysv -Wl,-Bsymbolic \
 -Wl,--version-script=$(PIN_HOME)/source/include/pin/pintool.ver \
 -L$(PIN_HOME)/intel64/lib \
 -L$(PIN_HOME)/intel64/lib-ext \
 -L$(PIN_HOME)/intel64/runtime/glibc \
 -L$(PIN_HOME)/extras/xed-intel64/lib \
 -L$(PROTO_HOME)/lib/ \
 -lpin -lxed -lpindwarf -ldl -lprotobuf -ljsoncpp

all: obj-intel64/gltrace_sim.so src/gltrace_sim_pb2.py

obj-intel64/%.o: src/%.cpp 
	g++ $(_CXXFLAGS) $(CXXFLAGS) -c -o $@ $<

obj-intel64/%.o: src/%.cc 
	g++ $(_CXXFLAGS) $(CXXFLAGS) -c -o $@ $<

src/gltrace_sim.pb.cc: src/gltrace_sim.proto
	$(PROTOC) --cpp_out ./ $<

src/gltrace_sim_pb2.py: src/gltrace_sim.proto
	$(PROTOC) --python_out ./ $<

obj-intel64/gltrace_sim.so: obj-intel64/gltrace_sim.pb.o obj-intel64/gltrace_sim.o obj-intel64/resource.o obj-intel64/cache.o 
	g++ -o $@ $^ $(_LDFLAGS) $(LDFLAGS)

clean:
	rm obj-intel64/*.o
	rm obj-intel64/*.so
	rm src/*.pb.*
