This is the DjVu 2.2 Reference Library.

This code is intended to:
	- define the DjVu format
	- allow GNU projects to develop code that renders and
	  displays, and updates DjVu documents
	- allow the creation of unoptimized images from pre-segmented
	  image sources.

This package does not contain value added libraries such as
our SDK, JB2 optimizer, segmenter, optimizer, or GUI products.

While the 2.2 reference library was very well designed and is 
a very useful guide for understanding DjVu technology, the 3.x
versions are more complete.  However, most of the 3.x innovations
are not backwards compatible with the 2.x versions of DjVu.  For that
reason, we advise against actively developing products with the 2.x code
base.

To build the reference library on a Unix platform:

	./configure
	make

To install: (as root)
	cp bin/* /usr/bin/.
	cp lib/* /usr/lib/.



