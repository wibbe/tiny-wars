CC := cc
WINDRES := windres
STRIP := strip

BUILD_DIR := bin
TINYWAR_EXE := ./$(BUILD_DIR)/TinyWar.exe
COMMON := -D_WIN32_WINNT=0x0501 -I./lib -I./mingw -I. -luser32 -lgdi32 -lwinmm -std=c11 -Wall

.PHONY: all run debug clean

all: tinywar

# Build rules for the trampoline application
tinywar: $(TINYWAR_EXE)	## Build the trampoline application

bin/main.o: main.rc res/*.png res/*.ini
	@mkdir -p $(BUILD_DIR)
	$(WINDRES) -i main.rc -o bin/main.o

$(TINYWAR_EXE): *.c *.h bin/main.o
	$(CC) -g main.c bin/main.o -o $(TINYWAR_EXE) $(COMMON)

release: *.c *.h bin/main.o
	$(CC) -O2 -DRELEASE_BUILD -mwindows main.c bin/main.o -o $(TINYWAR_EXE) $(COMMON)
	$(STRIP) $(TINYWAR_EXE)

run: $(TINYWAR_EXE)
	$(TINYWAR_EXE)

debug: $(TINYWAR_EXE)
	gdb $(TINYWAR_EXE)

clean:
	rm -rf $(BUILD_DIR)/*
