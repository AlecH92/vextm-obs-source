PROJECT := vextm-source
TARGET := $(PROJECT).dll

SRCS = vextm-source.c \
	   vextm-thread.c \
	   colorbars.c

OBS_SOURCE := /mnt/c/Users/Dave/git/obs-studio
OBS_INSTALL := '/mnt/c/Program Files (x86)/obs-studio'

CC = i686-w64-mingw32-gcc
STRIP = i686-w64-mingw32-strip

CFLAGS = -Wall
INCLUDES = -I$(OBS_SOURCE)/libobs
LIBS = -L$(OBS_INSTALL)/bin/32bit -static-libgcc -mwindows -Wl,-Bdynamic -lobs -Wl,-Bstatic -lpthread

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
	@cp $(TARGET) $(OBS_INSTALL)/obs-plugins/32bit/
	@mkdir -p $(OBS_INSTALL)/data/obs-plugins/$(PROJECT)/
	@cp -R data/* $(OBS_INSTALL)/data/obs-plugins/$(PROJECT)/

zipfile:: $(TARGET)
	@echo "Creating $(PROJECT).zip..."
	@mkdir -p dist/obs-plugins/32bit/
	@cp $(TARGET) dist/obs-plugins/32bit/
	@$(STRIP) dist/obs-plugins/32bit/$(TARGET)
	@mkdir -p dist/data/obs-plugins/$(PROJECT)/
	@cp -R data/* dist/data/obs-plugins/$(PROJECT)/
	@cd dist && zip -r ../$(PROJECT).zip * && cd ..

clean:
	rm -f $(TARGET)
	rm -f $(OBJS)
	rm -rf dist
	rm -f $(PROJECT).zip
