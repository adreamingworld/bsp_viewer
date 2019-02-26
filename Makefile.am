AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = -I m4 --install

# Files we want to be packaged in the tar ball distribution
EXTRA_DIST = 	README.md 

# Make one binary called <hello>
bin_PROGRAMS = bsp_viewer

# Sources used to create the <hello> binary
bsp_viewer_SOURCES = src/bsp.c

# Data we want to include with our package.
pkgdata_DATA = resources/icon.bmp

# pkgdatadir is usuallu the /usr/local/share
# where the above pkgdata_DATA will be stored
# we need to make it so the source code can 
# access this string. So we pass is via the
# compiler here.
bsp_viewer_CFLAGS = @SDL_CFLAGS@ -DDATA_PATH='"$(pkgdatadir)"' -Wall -Werror

bsp_viewer_LDADD = @SDL_LIBS@ @GL_LIBS@ @IL_LIBS@

uninstall-hook:
	rm -rf $(pkgdatadir)
