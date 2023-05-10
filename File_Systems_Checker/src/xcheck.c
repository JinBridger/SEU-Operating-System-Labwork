#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define stat xv6_stat // avoid clash with host struct stat

// Specify the xv6 path here!
#include "./xv6-public/fs.h"
#include "./xv6-public/param.h"
#include "./xv6-public/stat.h"
#include "./xv6-public/types.h"

int img_file;

struct superblock sblock;
struct dinode cur_inode;

int error_check_1(struct dinode nd) {
    if(nd.type != 0 && nd.type != T_DEV && nd.type != T_DIR && nd.type != T_FILE) 
        return 1;
    return 0;
}

int error_check_2(struct dinode nd) {
    uint bitmap_end = sblock.bmapstart + sblock.size / (8 * BSIZE);
    uint fs_end = sblock.size - 1;

    if(sblock.size % (8 * BSIZE) != 0)
        bitmap_end++;
    
    // Loop all direct ptrs
    for(uint i = 0; i < NDIRECT + 1; ++i) 
        if(nd.addrs[i] != 0 && (nd.addrs[i] < bitmap_end || nd.addrs[i] > fs_end)) 
            return 1;
        
    if(nd.addrs[NDIRECT] != 0) {
        uint ndirect_ptr;
        lseek(img_file, nd.addrs[NDIRECT] * BSIZE, SEEK_SET);
        for(uint i = 0; i < NINDIRECT; ++i) {
            read(img_file, &ndirect_ptr, sizeof(uint));
            if(ndirect_ptr != 0 && (ndirect_ptr < bitmap_end || ndirect_ptr > fs_end)) 
                return 1;
        }
    }
    
    return 0;
}

int error_check_3() {
    struct dinode root_inode;
    lseek(img_file, sblock.inodestart * BSIZE + sizeof(struct dinode), SEEK_SET);
    read(img_file, &root_inode, sizeof(struct dinode));

    if(root_inode.type != T_DIR)
        return 1;

    struct dirent dir_entry;
    for(uint i = 0; i < NDIRECT; ++i)
        if(root_inode.addrs[i] != 0) {
            lseek(img_file, root_inode.addrs[i] * BSIZE, SEEK_SET);
            for(uint dir_cnt = 0; dir_cnt < BSIZE / (sizeof(struct dirent)); ++dir_cnt) {
                read(img_file, &dir_entry, sizeof(struct dirent));

                if(dir_entry.inum != 0) 
                    if(strncmp(dir_entry.name, "..", DIRSIZ) == 0 && dir_entry.inum == 1)
                        return 0;
            }
            
        }

    return 1;
}

int error_check_4(struct dinode nd, uint inode_num) {
    int chk = 0;
    struct dirent dir_entry;
    for(uint i = 0; i < NDIRECT; ++i) {
        if(nd.addrs[i] != 0) {
            lseek(img_file, nd.addrs[i] * BSIZE, SEEK_SET);
            for(uint dir_cnt = 0; dir_cnt < BSIZE / (sizeof(struct dirent)); ++dir_cnt) {
                read(img_file, &dir_entry, sizeof(struct dirent));

                if(strncmp(dir_entry.name, ".", DIRSIZ) == 0 && dir_entry.inum == inode_num)
                    chk += 1;

                if(strncmp(dir_entry.name, "..", DIRSIZ) == 0)
                    chk += 1;
            }
        }
    }
    if(chk != 2)
        return 1;
    return 0;
}

int error_check_5(struct dinode nd) {
    for(uint i = 0; i < NDIRECT + 1; ++i) {
        if(nd.addrs[i] != 0) {
            uint usebit, shift_amt;

            lseek(img_file, sblock.bmapstart * BSIZE + nd.addrs[i] / 8, SEEK_SET);
            read(img_file, &usebit, sizeof(uint));

            shift_amt = nd.addrs[i] % 8;
            usebit >>= shift_amt;
            if((usebit & 1) == 0)
                return 1;
        }
    }

    if(nd.addrs[NDIRECT] != 0) {
        uint ndirect_ptr;
        for(uint i = 0; i < NINDIRECT; ++i) {
            lseek(img_file, nd.addrs[NDIRECT] * BSIZE + i * sizeof(uint), SEEK_SET);
            read(img_file, &ndirect_ptr, sizeof(uint));
            if(ndirect_ptr != 0) {
                uint usebit, shift_amt;

                lseek(img_file, sblock.bmapstart * BSIZE + ndirect_ptr / 8, SEEK_SET);
                read(img_file, &usebit, sizeof(uint));

                shift_amt = ndirect_ptr % 8;
                usebit >>= shift_amt;
                if((usebit & 1) == 0)
                    return 1;
            }
        }
    }

    return 0;
}

int error_check_6(uint* in_use) {
    lseek(img_file, sblock.bmapstart * BSIZE + sblock.size / 8 - sblock.nblocks / 8, SEEK_SET);

    uchar bits;
    uint cnt = sblock.bmapstart + 1;
    for(uint i = cnt; i < sblock.size; i += 8) {
        read(img_file, &bits, sizeof(uchar));

        for(uint offset = 0; offset < 8; ++offset, ++cnt) {
            if((bits >> offset) % 2) {
                if(in_use[cnt] == 0) {
                    return 1;
                }
            }
        }
    }

    return 0;
}

int error_check_7(struct dinode nd, uint *in_use) {
    for(uint i = 0; i < NDIRECT + 1; ++i) {
        if(nd.addrs[i] != 0) {
            if(in_use[nd.addrs[i]])
                return 1;
            in_use[nd.addrs[i]] = 1;
        }
    }
    return 0;
}

int error_check_8(struct dinode nd, uint *in_use) {
    if(nd.addrs[NDIRECT] != 0) {
        uint ndirect_ptr;
        for(uint i = 0; i < NINDIRECT; ++i) {
            lseek(img_file, nd.addrs[NDIRECT] * BSIZE + i * sizeof(uint), SEEK_SET);
            read(img_file, &ndirect_ptr, sizeof(uint));
            if(ndirect_ptr != 0) {
                if(in_use[ndirect_ptr] == 1)
                    return 1;
                in_use[ndirect_ptr] = 1;
            }
        }
    }
    return 0;
}

int error_check_9(uint inode_num) {
    struct dinode tmp_inode;
    for(uint i = 0; i < sblock.ninodes; ++i) {
        lseek(img_file, sblock.inodestart * BSIZE + i * sizeof(struct dinode), SEEK_SET);
        read(img_file, &tmp_inode, sizeof(struct dinode));

        if(tmp_inode.type == T_DIR && inode_num != i && i != 1) {
            for(uint j = 0; j < NDIRECT; ++i) {
                if(tmp_inode.addrs[j] != 0) {
                    struct dirent dir_entry;

                    lseek(img_file, tmp_inode.addrs[j] * BSIZE, SEEK_SET);
                    read(img_file, &dir_entry, sizeof(struct dirent));

                    if(dir_entry.inum == inode_num)
                        return 0;
                }
            }

            if(tmp_inode.addrs[NDIRECT] != 0) {
                uint addr;
                for(uint j = 0; j < NINDIRECT; ++j) {
                    lseek(img_file, tmp_inode.addrs[NDIRECT] * BSIZE + j * sizeof(uint), SEEK_SET);
                    read(img_file, &addr, sizeof(uint));

                    if(addr != 0) {
                        struct dirent dir_entry;

                        lseek(img_file, addr * BSIZE, SEEK_SET);
                        read(img_file, &dir_entry, sizeof(struct dirent));

                        if(dir_entry.inum == inode_num) 
                            return 0;
                    }
                }
            }

        }
    }
    return 1;
}

int error_check_10(struct dinode nd) {
    for(uint i = 0; i < NDIRECT; ++i) {
        if(nd.addrs[i] != 0) {
            struct dirent dir_entry;
            for(uint j = 0; j < BSIZE / sizeof(struct dirent); ++j) {
                lseek(img_file, nd.addrs[i] * BSIZE + j * sizeof(struct dirent), SEEK_SET);
                read(img_file, &dir_entry, sizeof(struct dirent));

                struct dinode tmp_nd;
                if(dir_entry.inum != 0) {
                    lseek(img_file, sblock.inodestart * BSIZE + dir_entry.inum * sizeof(struct dinode), SEEK_SET);
                    read(img_file, &tmp_nd, sizeof(struct dinode));

                    if(tmp_nd.type == 0) {
                        return 1;
                    }
                }
            }
        }
    }

    if(nd.addrs[NDIRECT] != 0) {
        uint addr;
        struct dirent dir_entry;
        for(uint i = 0; i < NINDIRECT; ++i) {
            lseek(img_file, nd.addrs[NDIRECT] * BSIZE + i * sizeof(uint), SEEK_SET);
            read(img_file, &addr, sizeof(uint));

            for(uint j = 0; j < BSIZE / sizeof(struct dirent); ++j) {
                lseek(img_file, addr * BSIZE + j * sizeof(struct dirent), SEEK_SET);
                read(img_file, &dir_entry, sizeof(struct dirent));

                struct dinode tmp_nd;
                if(dir_entry.inum != 0) {
                    lseek(img_file, sblock.inodestart * BSIZE + dir_entry.inum * sizeof(struct dinode), SEEK_SET);
                    read(img_file, &tmp_nd, sizeof(struct dinode));

                    if(tmp_nd.type == 0) {
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

int error_check_11(struct dinode nd, uint inode_num) {
    uint cnt = 0;
    struct dinode tmp_inode;
    for(uint i = 0; i < sblock.ninodes; ++i) {
        lseek(img_file, sblock.inodestart * BSIZE + i * sizeof(struct dinode), SEEK_SET);
        read(img_file, &tmp_inode, sizeof(struct dinode));

        if(tmp_inode.type == T_DIR) {
            for(uint j = 0; j < NDIRECT; ++j) {
                if(tmp_inode.addrs[j] != 0) {
                    struct dirent dir_entry;
                    for(uint dir_cnt = 0; dir_cnt < BSIZE / sizeof(struct dirent); ++dir_cnt) {
                        lseek(img_file, tmp_inode.addrs[j] * BSIZE + dir_cnt * sizeof(struct dirent), SEEK_SET);
                        read(img_file, &dir_entry, sizeof(struct dirent));

                        if(dir_entry.inum == inode_num)
                            cnt++;
                    }
                }
            }

            if(tmp_inode.addrs[NDIRECT] != 0) {
                uint addr;
                for(uint j = 0; j < NINDIRECT; ++j) {
                    lseek(img_file, tmp_inode.addrs[NDIRECT] * BSIZE + j * sizeof(uint), SEEK_SET);
                    read(img_file, &addr, sizeof(uint));

                    if(addr != 0) {
                        struct dirent dir_entry;
                        for(uint dir_cnt = 0; dir_cnt < BSIZE / sizeof(struct dirent); ++dir_cnt) {
                            lseek(img_file, addr * BSIZE + dir_cnt * sizeof(struct dirent), SEEK_SET);
                            read(img_file, &dir_entry, sizeof(struct dirent));

                            if(dir_entry.inum == inode_num)
                                cnt++;
                        }
                    }
                }
            }
        }
    }

    if(cnt != nd.nlink)
        return 1;

    return 0;
}

int error_check_12(struct dinode nd, uint inode_num) {
    uint cnt = 0;
    struct dinode tmp_inode;
    for(uint i = 0; i < sblock.ninodes; ++i) {
        lseek(img_file, sblock.inodestart * BSIZE + i * sizeof(struct dinode), SEEK_SET);
        read(img_file, &tmp_inode, sizeof(struct dinode));

        if(tmp_inode.type == T_DIR) {
            for(uint j = 0; j < NDIRECT; ++j) {
                if(tmp_inode.addrs[j] != 0) {
                    struct dirent dir_entry;
                    for(uint dir_cnt = 0; dir_cnt < BSIZE / sizeof(struct dirent); ++dir_cnt) {
                        lseek(img_file, tmp_inode.addrs[j] * BSIZE + dir_cnt * sizeof(struct dirent), SEEK_SET);
                        read(img_file, &dir_entry, sizeof(struct dirent));

                        if(dir_entry.inum == inode_num)
                            cnt++;
                    }
                }
            }

            if(tmp_inode.addrs[NDIRECT] != 0) {
                uint addr;
                for(uint j = 0; j < NINDIRECT; ++j) {
                    lseek(img_file, tmp_inode.addrs[NDIRECT] * BSIZE + j * sizeof(uint), SEEK_SET);
                    read(img_file, &addr, sizeof(uint));

                    if(addr != 0) {
                        struct dirent dir_entry;
                        for(uint dir_cnt = 0; dir_cnt < BSIZE / sizeof(struct dirent); ++dir_cnt) {
                            lseek(img_file, addr * BSIZE + dir_cnt * sizeof(struct dirent), SEEK_SET);
                            read(img_file, &dir_entry, sizeof(struct dirent));

                            if(dir_entry.inum == inode_num)
                                cnt++;
                        }
                    }
                }
            }
        }
    }

    if(cnt != 1)
        return 1;

    return 0;
}

int main(int argc, char* argv[]) {
    // Check arguments
    if(argc != 2) {
        fprintf(stderr, "Usage: xcheck <file_system_image>\n");
        return 1;
    }

    img_file = open(argv[1], O_RDONLY);
    if(img_file < 0) {
        fprintf(stderr, "image not found\n");
        return 1;
    }

    // Load superblock
    lseek(img_file, BSIZE * 1, SEEK_SET);
    read(img_file, &sblock, sizeof(sblock));

    // init in_use arr
    uint in_use[sblock.size];
    for(int i = 0; i < sblock.size; ++i)
        in_use[i] = 0;

    if(error_check_3()) {
        fprintf(stderr, "ERROR: root directory does not exist\n");
        close(img_file);
        exit(1);
    }

    // Loop all inodes
    for(uint inode_num = 0; inode_num < sblock.ninodes; ++inode_num) {
        // Load cur_inode
        lseek(img_file, sblock.inodestart * BSIZE + sizeof(struct dinode) * inode_num, SEEK_SET);
        read(img_file, &cur_inode, sizeof(struct dinode));

        if(error_check_1(cur_inode)) {
            fprintf(stderr, "ERROR: bad inode\n");
            close(img_file);
            exit(1);
        }

        if(cur_inode.type != 0) {
            if(error_check_2(cur_inode)) {
                fprintf(stderr, "ERROR: bad indirect address in inode\n");
                close(img_file);
                exit(1);
            }

            if(error_check_5(cur_inode)) {
                fprintf(stderr, "ERROR: address used by inode but marked free in bitmap\n");
                close(img_file);
                exit(1);
            }

            if(error_check_7(cur_inode, in_use)) {
                fprintf(stderr, "ERROR: direct address used more than once\n");
                close(img_file);
                exit(1);
            }

            if(error_check_8(cur_inode, in_use)) {
                fprintf(stderr, "ERROR: indirect address used more than once\n");
                close(img_file);
                exit(1);
            }

            if(cur_inode.type == T_DIR) {
                if(error_check_4(cur_inode, inode_num)) {
                    fprintf(stderr, "ERROR: directory not properly formatted\n");
                    close(img_file);
                    exit(1);
                }
                
                if(inode_num != 1){
                    if(error_check_9(inode_num)) {
                        fprintf(stderr, "ERROR: inode marked use but not found in a directory\n");
                        close(img_file);
                        exit(1);
                    }

                    if(error_check_12(cur_inode, inode_num)) {
                        fprintf(stderr, "ERROR: directory appears more than once in file system\n");
                        close(img_file);
                        exit(1);
                    }
                }

                if(error_check_10(cur_inode)) {
                    fprintf(stderr, "ERROR: inode referred to in directory but marked free\n");
                    close(img_file);
                    exit(1);
                }

                
            }

            if(cur_inode.type == T_FILE) {
                if(error_check_11(cur_inode, inode_num)) {
                    fprintf(stderr, "ERROR: bad reference count for file\n");
                    close(img_file);
                    exit(1);
                }
            }
        }
    }

    if(error_check_6(in_use)) {
        fprintf(stderr, "ERROR: bitmap marks block in use but it is not in use\n");
        close(img_file);
        exit(1);
    }

    return 0;
}