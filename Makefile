all: scottfree

glkterm: glkterm/libglkterm.a

glkterm/libglkterm.a glkterm/Make.glkterm:
	cd glkterm && make

scottfree/scottfree: glkterm/libglkterm.a glkterm/Make.glkterm scottfree/Makefile
	cd scottfree && make

scottfree: scottfree/scottfree
	ln -sf scottfree/scottfree scott

clean:
	rm -rf scottfree/*.o scottfree/scottfree
	cd glkterm && make clean
