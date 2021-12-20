Include_Dir := Include/

CPP_FILES := $(shell find $(shell find source/ -type f -name '*.cpp') -name '*.c*')

all: 
	g++ $(CPP_FILES) -s -Os -I $(Include_Dir) -o JKRArchiveTools.exe 