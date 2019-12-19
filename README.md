# depklite
Command line tool to extract data from PKLITE compressed DOS executables. Data is written to another file. This is useful for searching for strings or to disassemble and examine the code.

The decompression routines are based on  @NY00123's [refkleen](https://github.com/NY00123/refkeen) which in turn were ported from @afrtiz1's [OpenTESArena](https://github.com/afritz1/OpenTESArena). @Afritz1 gives credit to @dozayon's [excellent documentation](https://github.com/afritz1/OpenTESArena/blob/master/docs/pklite_specification.md) of the PKLITE V1.12 compression format.

This program is released under the same MIT license used by refkleen's depklite implementation.

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
