## Minotar
Minotar is a MINimal memory Overhead TARball extraction library.  It accomplishes this by only holding 560 bytes at a time in memory and parsing the incoming data as a stream.  Each file is parsed and written to disk in-band.  A system which has little memory can effectively receive a file from an external source without needing enough room to store both the packaged tarball on disk or in memory at the same time as the  divided file data.  This mechanism is very useful for things such as firmware updates or live file-based data streams.

It is very simple to wrap Minotar and give gzip functionality.  See the examples directory for usage.

As different applications supporting tar contain very fragmented extensions, it would be difficult to support them all.  Currently this library supports basic tarball functionality and tarball ustar functionality as specified in the IEEE spec.  I've tested this library against packages compressed with GNU Tar and BSD Tar to verify the functionality.
