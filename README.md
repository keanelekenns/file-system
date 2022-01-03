# file-system
This is a mock file system that was completed as an assignment for an operating systems course in the spring of 2019.

Here are some quick notes:
- The assignment description (project requirements) can be found in LLFS.pdf.
- The io directory contains the implementation of the file system code.
- The disk directory contains a basic interface to a virtual disk that writes and reads blocks to a vdisk (which is just a plain binary file).
- The apps directory contains all of the tests that interact with the file system and demonstrate its functionality.
- The tests can be run within the apps directory by running the test.sh script (which accompanies the tests with their appropriate hex dumps). From the main directory run ```cd apps && ./test.sh```
- The output of the test.sh script can also be seen in output.txt

The following is the write up for the original assignment submission...

## File System Project

Keanelek Enns

Spring 2019

RUNNING MY SCRIPT (test.sh):
The bash script must be run from within the apps folder. If the
script needs to be given executable permissions; do this with "chmod +x test.sh".
Then simply run the script with "./test.sh" and it will make all of the test files
as well as run them (it will produce a large amount of output, so just scroll to the 
top and work your way down). It runs each of the test files (which have commentary inside
them about what they are doing) and then performs a hexdump afterwards so that the
markers can see the actions performed actually worked (it also prints a separating
line between each section for clarity). Please note that I spent a lot of time making
sure that dynamically allocated memory was properly freed up (which was not easy in the
write file), so it is worth noting that if you run "valgrind ./test01" (or test02,3,etc.),
you will see that there are no memory leeks (test03 is especially good for seeing this).
I also have an output file which is simply a text file containing what I see as output
when I run my script. This is in case something weird happens.

IMPLEMENTATION OVERVIEW:
My implementation ended up being more of a LUFS (Little Unix File System).
The main reason for this is that I wasn't sure I liked the
thought of having all my work be in a buffer without saving it to the disk for a long
period of time (I understand how that improves performance in an actual file system
by reducing the cost and frequency of writes, but in this assignment I wasn't super
worried about performance). The thing about it that bothered me is that you would
have to store changes to data across function calls. Then you would need to keep track
of what has been modified so you don't use old data blocks in functions. 
Another thing I thought to be strange was the moving inode map, if 
there is a stationary checkpoint region that needs to be updated with the location
of the inode map, why not just make the inode map the stationary checkpoint and update
it? It seems like an unecessary extra step (apparently my way is the way it is actually
done, but the slides say otherwise).

My implementation is unlike UFS in that there is no freelist;
I maintain the free block vector described in the write up.

I understand that physically this implementation would require seeking back and forth between
the inode area and data block area.

INODES:
I have an inode area immediately after block 9 (Block indexing starts at 0) and the range is from
block 10 to NUM_INODES + 9 inclusively. Inode ids are determined by their block number - 9.
Therefore to index inodes we use inode id + 9 (Inode ids start at 1). Inode 1 (at block 10)
is the inode for the root directory and is initialized in init_LLFS().
I made my inodes occupy an entire block on their own because that is what I was told
was a requirement earlier on. I may change this later on, but for now my system is already using that
format. I ended up deciding to keep it that way. I have shown that I know how the indexing would have
worked as it would have been similar to directories (which also have 16 32-byte entries). If nothing
else, it makes the hexdump more readable because an inode is its own block (same format of inodes as
in asignment description).

GENERAL NOTES:
When I delete an entry in a directory, I replace it with the last entry. This
makes all of the entries contiguous in memory.

The way I determine the index of the next free block is to literally do a linear 
search through the free block for the first set bit. This means blocks are allocated
predictably.

Because directory sizes are limited by the number of inodes (one per directory entry),
and in my implementation there are 128 inodes, all of the inodes in the system can 
fit into the first 8 direct blocks of a directory. For this reason, I have decided not
to include logic for indirect & double indirect blocks for directories specifically.
Consider the case where we use every available block in the system for inodes. This would
be 4086 inodes and these would fit within 4096/16 = 256 directory blocks, the first indirect
block would not be filled and we wouldn't even have any data blocks. This illustrates
how unnecessary using indirect and double indirect blocks in directories is in this file system.
I will use the last two block pointers as direct block pointers so the maximum number of inodes
that can fit in a directory is 192. There is an assert statement restricting the value of 
NUM_INODES.

WRITE FUNCTION:
This function is disgusting. It is a result of my early design
decisions, that I didn't realize would be so terrible to write
(dynamically allocating data, checking for failure every time I
read or write a block, etc.). But so far it has held up to the 
testing I have done on it. And I realized that I was already 
implementing delete in it (if the new data length is less than
the old, it goes through and deletes/frees the old data blocks).
When we write to a file, we reuse whatever data blocks we can/need
from the old file. The implementation of this function is such that
if we delete files or write anything to them, we remove all old data,
this means any blocks with a nonzero value in it, are in use (this
is helpful for robustness).

ROBUSTNESS:
I was having a hard time knowing specifically what to do to get the marks for this.
Since I am not implementing a log structured file system, I don't write the data 
to a buffer and dump it out when it's full. Instead, in all of the functions I made
an effort to write all of the updates to blocks at the very end. This means as I modify
things, if I detect an error and have to exit the function, no changes will be made, 
rather than being in the middle of making changes and having them corrupt the system.
It is possible for the system to crash in between each call to write_block,
but it would be the same case for LLFS too (only it would be during the offloading of the buffer).
So the main difference is that I write all my updates at the end of a function, not at
the end of filling a buffer. I do have a simple fsck() function that basically checks each of
the bits in free block and sees if the corresponding blocks have data in them. If the free
block says it should be free, but it has data, then we unset the bit. If the free block
says it is allocated, but it has nothing in it, we free it. This is far from a perfect solution
(or even a good one), but I'm out of time. I have come to realize that you can never be 100% sure
of what goes where. You have to make some guesses, so I do understand that tradeoff. Also, I was
part way through a function that was going to walk through the entire filesystem and compare each
of the blocks in each inode to the contents of the free block. I know how to write this function,
but I simply don't have time. Hopefully it's clear that I understand the difficulties surrounding
robustness and that I made an effort to write to the disk all at once specifically for the sake of 
robustness. The consistent order of updates (free block, inodes, data blocks) would help for knowing
what is more accurate (although I started to realize that you might not want to update the free block
first). 

disk_controller.c:
I decided that the read and write functions would load 512 bytes
from and into memory regardless of the properties of the char* buffer passed
in. This means it is the responsibility of the caller (the file system)
to ensure that garbage does not get written to the disk and loaded from 
the disk.
init_vdisk() simply moves the file position to the appropriate size of vdisk
in order to increase its size. This means there is potential for garbage
to be in vdisk, although I haven't seen any in my testing and I don't think
it matters if there is since we will simply overwrite blocks.

TEST01:
Note that I merged the described test01.c and test02.c into this as I had to
create and mount a disk in order to perform reads and writes to blocks. So this
tests both of those features (check out the comments in the code for some reasoning
behind the notations for calling write_block and read_block). Also, this test is
executing from the perspective of the filesystem itself (it uses the disk controller
directly), whereas the later tests are simulating user applications using the filesystem.

TEST02:
This test simply formats the vdisk with my version of LLFS (which is
goal 1 of our assignment [20 marks]). The print out of the test
describes the formating. I stuck to the format that the writeup gave
(i.e. a super block followed by a free block, and I reserved blocks 
0-9 even though I didn't end up using most of them). The magic number
in the superblock is arbitrary because I never got an explanation as 
to what that number should mean in the context of our filesystem.

TEST03:
Creates a file and writes 1.5MB of data to it.
The hexdump is long, but it's pretty cool.

TEST04:
This reads the file that was written to in test03. Test03 and test04
combined, check off the "reading and writing of files in the root
directory [20 marks]" objective. Instead of printing the entire buffer,
I just print the buffer at the first and last indices that we expect
the '$' symbol to be in, then I print the next char in the buffer. Since I 
try to read 2MB, it demonstrates that the read will go up to the minimum
of the length given and the file's size.

TEST05:
Demonstrates the "creation of sub-directories [10 marks]". Notice
that it is not limited to 4 levels of sub-directories (I don't know
why doing more than 4 levels is difficult). If you observe the inodes
and their filesizes, then look at their corresponding data blocks, you
will see that everything lines up. Also, the first entry for any 
directory is its own inode id followed by '.', and the second entry for any
directory other than the root is its parent directory's inode id followed by "..".
Note also that the periods don't show up in the hex dump as the period is the
default character, but you can see the values 0x2e for periods.

TEST06:
Demonstrates "reading and writing of files in any directory [15 marks]".
This test depends on the results from test05.
I write a string to a file called report.txt in the /a3 directory, then I read only a small 
portion of it to show that you don't need to read the whole file into your buffer.

TEST07:
Demonstrates "deletion of files and directories [10 marks]".
This test depends on the results from test06 and test05 because
it deletes the file and directories made in those tests. Notice
that the directory /a3 still has continuous data even though 
/a3/io was before /a3/disk in its entries.

And of course, all of the tests make up the last goal, "Submit test files used to exercise functionality [10 marks]".
Thank you for marking this, have an awesome day!
