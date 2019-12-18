# depklite
Command line tool to extract data from DOS executables compressed with PKlite. Can be handy to search for strings or to disassemble and examine the code.

This uses the decompression routines from [refkleen](https://github.com/NY00123/refkeen) and [OpenTESArena](https://github.com/afritz1/OpenTESArena) to extract the data from a PKlite compressed .EXE file and write it to another file. It is under the same MIT license used by refkleen's implementation.

## Usage

depklite - extract data from DOS executables compressed with PKlite.

Usage:  **depklite**  _[-dv] [-o &lt;outfile>]_  foo.exe  _[offset]_

        foo.exe: name of the executable file to decompress.

Options:

        offset:  start of compressed data. (Default is 0).
        -d, --decrypt           Enable decryption of data.
        -o, --output <outfile>  Write to outfile instead of foo.exe.dep.
        -v, --verbose [=<0|1>]  Be more verbose.

## Notes

* Ideally, this program would create a valid DOS executable which is uncompressed. However, writing the DOS header and putting the code in the right place to execute isn't trivial and serves no useful purpose that I can think of.  

* This does not detect the start of the compressed data. It defaults to zero, which is definitely wrong, but is more likely to have all the data extracted. 800 is where the Commander Keen 1.00 data was. 752 was what Elder Scrolls Arena used.

* Decryption routines were not tested.

* The decoding routines use a fixed size buffer (current 4MiB) for storing output, so anything beyond that is truncated. I doubt this will be a limitation, but if it is, let me know.
