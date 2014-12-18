NACL_SDK_ROOT=~/Documents/nacl_sdk/pepper_39
DJVULIBRE_ROOT=~/Documents/djvulibre-3.5.25

NACL_FINALIZE=$(NACL_SDK_ROOT)/toolchain/linux_pnacl/bin/pnacl-finalize

CXX=$(NACL_SDK_ROOT)/toolchain/linux_pnacl/bin/pnacl-clang++

CXXINCLUDE=-I $(NACL_SDK_ROOT)/include -I $(DJVULIBRE_ROOT)/libdjvu
CXXFLAGS=-std=gnu++11 $(CXXINCLUDE)

LDFLAGS=-L $(NACL_SDK_ROOT)/lib/pnacl/Release/ -L $(DJVULIBRE_ROOT)/libdjvu/.libs
LIBS=-l ppapi_cpp -l ppapi -l pthread -l djvulibre

CXXCPP=-D THREADMODEL=POSIXTHREADS -D HAVE_STDINT_H -D UPDATE_EXPORTS -D HAVE_NAMESPACES -D AUTOCONF -D HAVE_STDINCLUDES

SOURCES=module.cc loader/url_download_stream.cc loader/url_loader_handler.cc decoder/decoder.cc

all: final

final: graphics_nacl
	$(NACL_FINALIZE) graphics_nacl.pexe -o graphics_nacl.final.pexe

graphics_nacl:
	$(CXX) $(SOURCES) $(CXXCPP) $(CXXFLAGS) $(LDFLAGS) $(LIBS) -o graphics_nacl.pexe

clean:
	rm -rf graphics_nacl.pexe graphics_nacl.final.pexe

