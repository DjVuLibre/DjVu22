This is the DjVu 2.2 Reference Library.

This code is intended to:
	- define the DjVu format
	- allow GNU projects to develop code that renders and
	  displays, and updates DjVu documents
	- allows the creation of unoptimized images from pre-segmented
	  image sources.

We this package does not contain value added libraries, like
our SDK, JB2 optimizer, segmenter, optimizer, or GUI products.
Even if we were to release these additional items as open source,
most likely, patent rights would exclude there use in the open
source community.

While the 2.2 reference library was very well designed, and is still
a very useful guide for understanding DjVu technology.  The 3.x
versions are much better.  Unfortunately, most of the 3.x innovations
are not backwards compatible with the 2.x versions of DjVu.  For that
reason, we advice against actively developing products with the 2.x code
base.

To build the reference library on a Unix platform:

	./configure
	make

To install: (as root)
	cp bin/* /usr/bin/.
	cp lib/* /usr/lib/.



