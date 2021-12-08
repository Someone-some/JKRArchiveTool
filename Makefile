
Include_Dir := Include/

CPP_FILES := $(shell find $(shell find source/ -type f -name '*.cpp') -name '*.c*')

all: 
	g++ $(CPP_FILES) -I $(Include_Dir) -o RARCTools.exe 