NACL_FINALIZE=$(NACL_SDK_ROOT)/toolchain/linux_pnacl/bin/pnacl-finalize
NACL_TRANSLATE=$(NACL_SDK_ROOT)/toolchain/linux_pnacl/bin/pnacl-translate
NACL_COMPRESS=$(NACL_SDK_ROOT)/toolchain/linux_pnacl/bin/pnacl-compress

CXX=$(NACL_SDK_ROOT)/toolchain/linux_pnacl/bin/pnacl-clang++

CXXINCLUDE=-I $(NACL_SDK_ROOT)/include/pnacl -I $(NACL_SDK_ROOT)/include -I $(DJVULIBRE_ROOT) -I $(DJVULIBRE_ROOT)/libdjvu -I $(DDJVU_ROOT)/src
CXXFLAGS=-std=gnu++11 $(CXXINCLUDE)

LDFLAGS=-L $(NACL_SDK_ROOT)/lib/pnacl/Release/ -L $(DJVULIBRE_ROOT)/libdjvu/.libs
LIBS=-l ppapi_cpp -l ppapi -l pthread -l djvulibre -l png -l z

CXXCPPOLD=-D THREADMODEL=POSIXTHREADS -D HAVE_STDINT_H -D UPDATE_EXPORTS -D HAVE_NAMESPACES -D AUTOCONF -D HAVE_STDINCLUDES
CXXCPP=-D HAVE_CONFIG_H -D THREADMODEL=POSIXTHREADS -D NDEBUG

DECODER_SOURCES=src/decoder/module.cc src/loader/url_download_stream.cc src/loader/url_loader_handler.cc src/decoder/decoder.cc src/base64/base64.cc

#TODO (ilia) don't forget to compress
all: decoder-compress

debug: decoder-debug

decoder-compress: decoder-final
	$(NACL_COMPRESS) graphics-nacl-decoder.final.pexe -o graphics-nacl-decoder.release.pexe

decoder-final: graphics-nacl-decoder
	$(NACL_FINALIZE) graphics-nacl-decoder.pexe -o graphics-nacl-decoder.final.pexe

graphics-nacl-decoder:
	$(CXX) -O2 $(DECODER_SOURCES) $(CXXCPP) $(CXXFLAGS) $(LDFLAGS) $(LIBS) -o graphics-nacl-decoder.pexe

decoder-debug: graphics-nacl-debug
	$(NACL_TRANSLATE) --allow-llvm-bitcode-input graphics-nacl-decoder-debug.pexe -arch x86-32 -o graphics-nacl-decoder-debug_x86_32.nexe
	$(NACL_TRANSLATE) --allow-llvm-bitcode-input graphics-nacl-decoder-debug.pexe -arch x86-64 -o graphics-nacl-decoder-debug_x86_64.nexe

graphics-nacl-debug:
	$(CXX) $(DECODER_SOURCES) $(CXXCPP) $(CXXFLAGS) $(LDFLAGS) $(LIBS) -g -o graphics-nacl-decoder-debug.pexe

clean:
	rm -rf graphics-nacl-decoder.pexe graphics-nacl-decoder.final.pexe graphics-nacl-renderer.pexe graphics-nacl-renderer.final.pexe graphics-nacl-decoder-debug.pexe graphics-nacl-decoder-debug_x86_32.nexe graphics-nacl-decoder-debug_x86_64.nexe graphics-nacl-decoder-debug.bc

