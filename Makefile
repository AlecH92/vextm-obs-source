PROJECT := vextm-source
TARGET := $(PROJECT).dll

SRCS = vextm-source.c \
	   vextm-thread.c \
	   colorbars.c

OBS_SOURCE := /mnt/c/Users/Alec/Documents/GitHub/obs-studio
OBS_INSTALL := '/mnt/c/Program Files/obs-studio'

CC = x86_64-w64-mingw32-gcc-posix
STRIP = x86_64-w64-mingw32-strip

CFLAGS = -m64 -Wall
INCLUDES = -I$(OBS_SOURCE)/libobs
LIBS = -L$(OBS_INSTALL)/bin/64bit -static-libgcc -mwindows -Wl,-Bdynamic -lobs -Wl,-Bstatic -lpthread

OBJS += ${SRCS:.c=.o}

.PHONY: install clean all

all:: $(TARGET)

$(TARGET): $(OBJS)
	@echo "Linking $@..."
	@$(CC) -shared $(LIBS) -o $@ $(OBJS)

.c.o:
	@echo "Compiling $<..."
	@$(CC) -c $< -o $@ $(CFLAGS) $(CXXFLAGS) $(INCLUDES)

install:: $(TARGET)
	@echo "Installing to $(OBS_INSTALL)"
	@cp $(TARGET) $(OBS_INSTALL)/obs-plugins/64bit/
	@mkdir -p $(OBS_INSTALL)/data/obs-plugins/$(PROJECT)/
	@cp -R data/* $(OBS_INSTALL)/data/obs-plugins/$(PROJECT)/

zipfile:: $(TARGET)
	@echo "Creating $(PROJECT).zip..."
	@mkdir -p dist/obs-plugins/64bit/
	@cp $(TARGET) dist/obs-plugins/64bit/
	@$(STRIP) dist/obs-plugins/64bit/$(TARGET)
	@mkdir -p dist/data/obs-plugins/$(PROJECT)/
	@cp -R data/* dist/data/obs-plugins/$(PROJECT)/
	@cd dist && zip -r ../$(PROJECT).zip * && cd ..

clean:
	rm -f $(TARGET)
	rm -f $(OBJS)
	rm -rf dist
	rm -f $(PROJECT).zip
