NACL_FINALIZE=$(NACL_SDK_ROOT)/toolchain/linux_pnacl/bin/pnacl-finalize

CXX=$(NACL_SDK_ROOT)/toolchain/linux_pnacl/bin/pnacl-clang++

CXXINCLUDE=-I $(NACL_SDK_ROOT)/include -I $(DJVULIBRE_ROOT)/libdjvu -I $(DDJVU_ROOT)
CXXFLAGS=-std=gnu++11 $(CXXINCLUDE)

LDFLAGS=-L $(NACL_SDK_ROOT)/lib/pnacl/Release/ -L $(DJVULIBRE_ROOT)/libdjvu/.libs
LIBS=-l ppapi_cpp -l ppapi -l pthread -l djvulibre -l png -l z

CXXCPP=-D THREADMODEL=POSIXTHREADS -D HAVE_STDINT_H -D UPDATE_EXPORTS -D HAVE_NAMESPACES -D AUTOCONF -D HAVE_STDINCLUDES

DECODER_SOURCES=src/decoder/module.cc src/loader/url_download_stream.cc src/loader/url_loader_handler.cc src/decoder/decoder.cc src/base64/base64.cc

RENDERER_SOURCES=src/renderer/module.cc

#TODO (ilia) don't forget to compress
all: decoder-final renderer-final

decoder-final: graphics-nacl-decoder
	$(NACL_FINALIZE) graphics-nacl-decoder.pexe -o graphics-nacl-decoder.final.pexe

graphics-nacl-decoder:
	$(CXX) $(DECODER_SOURCES) $(CXXCPP) $(CXXFLAGS) $(LDFLAGS) $(LIBS) -o graphics-nacl-decoder.pexe

renderer-final: graphics-nacl-renderer
	$(NACL_FINALIZE) graphics-nacl-renderer.pexe -o graphics-nacl-renderer.final.pexe

graphics-nacl-renderer:
	$(CXX) $(RENDERER_SOURCES) $(CXXCPP) $(CXXFLAGS) $(LDFLAGS) $(LIBS) -o graphics-nacl-renderer.pexe

clean:
	rm -rf graphics-nacl-decoder.pexe graphics-nacl-decoder.final.pexe graphics-nacl-renderer.pexe graphics-nacl-renderer.final.pexe

