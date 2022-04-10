Include_Dir := Include

all: 
	g++ Source/*.cpp -s -Os -I $(Include_Dir) -o JKRArchiveTools.exe 