#define _LARGEFILE64_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <linux/fs.h>
#include <sys/types.h>
#include <math.h>
#include <unistd.h>
#include <ext2fs/ext2_fs.h>
#include <time.h>

unsigned int block_size;


char * con_tm(unsigned int t){
	time_t tmp = t;
	struct tm *real_time;
	time(&tmp);
	return asctime(localtime(&tmp));		
}


int main(int argc, char*argv[]){
	int fd  = open(argv[1], O_RDONLY);
	int count, i;
	struct ext2_super_block sb;
	struct ext2_group_desc bgdesc[1024];
	struct ext2_inode fos;
	int ngroups, inodeno, finode, bgno;
	if(argc!=3) return 1;
	if(fd == -1){
		perror("readsuper:");
		exit(errno);
	}
	inodeno = atoi(argv[2]);
	lseek64(fd, 1024, SEEK_SET);
	printf("size of super block = %lu\n", sizeof(struct ext2_super_block));

	count = read(fd, &sb, sizeof(struct ext2_super_block));
	printf("Magic : %x\n", sb.s_magic);
	printf("Inodes Count: %d\n", sb.s_inodes_count);
	printf("Size of BG DESC: %lu\n", sizeof(struct ext2_group_desc));
	printf("Inodes per group: %d\n", sb.s_inodes_per_group);

	block_size = 1024 << sb.s_log_block_size;
	bgno = ceil((inodeno - 1) / sb.s_inodes_per_group);
	finode = (inodeno - 1) % sb.s_inodes_per_group;

	lseek64(fd, 1024 + block_size + bgno * sizeof(bgdesc), SEEK_SET);
	count = read(fd, &bgdesc, sizeof(struct ext2_group_desc));

	printf("Inode table : %d\n", bgdesc[bgno].bg_inode_table);
	lseek64(fd, bgdesc[bgno].bg_inode_table * block_size + finode * sb.s_inode_size, SEEK_SET);

	__u16 modeig;
	count = read(fd, &modeig, sizeof(__u16));
	lseek64(fd, -2, SEEK_CUR);
	count = read(fd, &fos, sizeof(fos));

	printf("MODE : %u\n", modeig);
	printf("Inode Number: %d\n", inodeno);
	printf("File mode: %u\n", fos.i_mode);
	printf("Owner Uid: %d\n", fos.i_uid);
	printf("Size: %d\n", fos.i_size);
	printf("Access time: %s", con_tm(fos.i_atime));
	printf("Creation time: %s", con_tm(fos.i_ctime));
	printf("Modification time: %s", con_tm(fos.i_mtime));
	printf("Deletion time:  %s", con_tm(fos.i_dtime));
        printf("Group ID: %d\n", fos.i_gid);
        printf("No of Links: %d\n", fos.i_links_count);
        printf("Block Count: %d\n", fos.i_blocks);

	for (int i = 0; i < fos.i_blocks; i++)
		printf("%d, ", fos.i_block[i]);

        printf("File Flag: %u\n", fos.i_flags);
	close(fd);
	return 0;
}
