#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "lib_tar.h"

/**
 * You are free to use this file to write tests for your implementation
 */

void debug_dump(const uint8_t *bytes, size_t len) {
    for (int i = 0; i < len;) {
        printf("%04x:  ", (int) i);

        for (int j = 0; j < 16 && i + j < len; j++) {
            printf("%02x ", bytes[i + j]);
        }
        printf("\t");
        for (int j = 0; j < 16 && i < len; j++, i++) {
            printf("%c ", bytes[i]);
        }
        printf("\n");
    }
}




void test_list(int fd, char *path) {
    size_t *no_entries = malloc(sizeof(size_t)) ;
    char** entries = malloc(sizeof(char*)*10) ;
    for (int i = 0 ; i<10 ; i++) entries[i] = malloc(sizeof(char)*100) ;
    int check = list(fd, path, entries, no_entries) ;
    printf("list() returned %d for %s and the %zu found files are :\n", check, path, *no_entries) ;
    for (int i = 0 ; i<*no_entries ; i++) {
        printf("%s\n", entries[i]) ;
    }
}

int main(int argc, char **argv) {
    //printf("the size of a header is %ld\n", sizeof(tar_header_t)) ;
    if (argc < 2) {
        printf("Usage: %s tar_file\n", argv[0]);
        return -1;
    }

    int fd = open(argv[1] , O_RDONLY);
    if (fd == -1) {
        perror("open(tar_file)");
        return -1;
    }

    //int ret = check_archive(fd);
    //printf("check_archive returned %d\n", ret);

    // printing all files in the archive in the order we read it
    //printf("The files in the tar archive are :\n") ;
    tar_header_t* header = (tar_header_t*) malloc(sizeof(tar_header_t)) ;
    lseek(fd, 0, SEEK_SET) ;
    read(fd, header, 512) ;
    int offset ;
    while (strlen(header->name) != 0) {
        //printf("%s\n", header->name) ;
        offset = (TAR_INT(header->size)/512)*512;
        if (TAR_INT(header->size)%512 != 0) offset += 512 ;
        lseek(fd, offset, SEEK_CUR) ;
        read(fd, header, 512) ;
    }

    // TEST ON EXISTS
    char* path = "dos_ex" ;
    //printf("\nTests on exists() :\n") ;
    //printf("exists returned %d for %s\n", exists(fd, path), path) ;
    
    
    uint8_t* des = (uint8_t*)malloc(sizeof(uint8_t)*33);
    size_t len = 33;
    ssize_t r = read_file(fd, path, 0, des, &len);
    
    uint8_t* des2 = (uint8_t*)malloc(sizeof(uint8_t)*33);
    size_t len2 = 33;
    ssize_t r2 = read_file(fd, "fichier4.txt", 0, des2, &len2);
    printf("symlink\n");
    for (int i = 0; i < 33; i++){
    	printf("%c", des[i]);
    } 
    printf("\npas symlink\n");
    for (int i = 0; i < 33; i++){
    	printf("%c", des2[i]);
    }
    printf("\nr = %ld\n", r);
    printf("r2 = %ld\n", r2);
    printf("len = %ld\n", len);
    printf("len2 = %ld\n", len2);
    // TEST ON IS_FILE
    //printf("\nTests on is_file() :\n") ;
    //printf("is_file returned %d for %s\n", is_file(fd, path), path) ;

    // TEST ON IS_DIR
    //printf("\nTests on is_dir() :\n") ;
    //path = "dossier1/" ;
    //printf("is_dir returned %d for %s\n", is_dir(fd, path), path) ;

    // TEST ON IS_SYMLINK
    printf("\nTests on is_symlink() :\n") ;
    path = "example.txt" ;
    is_symlink(fd, path); 
    
    //printf("is_dir returned %d for %s\n", is_symlink(fd, path), path) ;

    // TEST ON LIST
    //printf("\nTests on list() :\n") ;
    //test_list(fd, "dos_ex") ;
    //test_list(fd, "dossier1/") ;
    return 0;
}
