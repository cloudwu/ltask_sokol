SOKOL_INC=-I sokol
LUAINC=`pkgconf lua --cflags`
LUALIB=`pkgconf lua --libs`

all : app.exe message.dll
	cd ltask && $(MAKE)

CFLAGS=-Wno-unknown-pragmas
LDFLAGS=-lkernel32 -luser32 -lshell32 -lgdi32 -ldxgi -ld3d11 -Wl,-subsystem,windows

app.exe : entry.c
	gcc -g -Wall -o $@ $< $(SOKOL_INC) $(CFLAGS) $(LDFLAGS) $(LUAINC) $(LUALIB)
	
message.dll : message.c
	gcc -g -Wall --shared -o $@ $< $(LUAINC) $(LUALIB)
	
clean :
	rm -f app.exe message.dll
	