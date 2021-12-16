#include "lib_tar.h"

/**
 * Checks whether the archive is valid.
 *
 * Each non-null header of a valid archive has:
 *  - a magic value of "ustar" and a null,
 *  - a version value of "00" and no null,
 *  - a correct checksum
 *
 * @param tar_fd A file descriptor pointing to the start of a file supposed to contain a tar archive.
 *
 * @return a zero or positive value if the archive is valid, representing the number of non-null headers in the archive,
 *         -1 if the archive contains a header with an invalid magic value,
 *         -2 if the archive contains a header with an invalid version value,
 *         -3 if the archive contains a header with an invalid checksum value
 */
int check_archive(int tar_fd) {
    tar_header_t* header = (tar_header_t*)malloc(512);
    lseek(tar_fd, 0, SEEK_SET) ;
    read(tar_fd, header, 512);
    int offset;
    int nbr = 0;
    while (strlen(header->name) != 0){

	    //magic value
        if (memcmp(header->magic, TMAGIC, TMAGLEN) != 0) return -1;

        //version
        if (memcmp(header->version, TVERSION, TVERSLEN) != 0) return -2;

        //checksum
	    unsigned int sum = 0;
	    char* h = (char*) header;
	    for (int i = 0; i < 512; i++){
	        if (i>= 148 && i < 156){
	    	    sum += (unsigned int) ' ';
	        }else{
	    	    sum += (unsigned int) h[i];
	        }
	    }
	    if ( TAR_INT(header->chksum) != sum){
	        return -3;
	    }
	    offset = (TAR_INT(header->size)/512)*512;
        if (TAR_INT(header->size)%512 != 0) offset += 512 ;
        lseek(tar_fd, offset, SEEK_CUR) ;
        read(tar_fd, header, 512) ;
        nbr++;
    } 
    return nbr;
}
/**
 * Checks whether an entry exists in the archive.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive,
 *         any other value otherwise.
 */
int exists(int tar_fd, char *path) {
    tar_header_t* header = (tar_header_t*) malloc(sizeof(tar_header_t)) ;
    lseek(tar_fd, 0, SEEK_SET) ;
    read(tar_fd, header, 512) ;
    int offset ;
    while (strlen(header->name) != 0) {
        if (strcmp(header->name, path) == 0) {
            return header->typeflag + 10 ;
        }
        offset = (TAR_INT(header->size)/512)*512;
        if (TAR_INT(header->size)%512 != 0) offset += 512 ;
        lseek(tar_fd, offset, SEEK_CUR) ;
        read(tar_fd, header, 512) ;
    }
    return 0;

}

/**
 * Checks whether an entry exists in the archive and is a directory.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive or the entry is not a directory,
 *         any other value otherwise.
 */
int is_dir(int tar_fd, char *path) {
    return exists(tar_fd, path) - 10 == DIRTYPE ;

}

/**
 * Checks whether an entry exists in the archive and is a file.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive or the entry is not a file,
 *         any other value otherwise.
 */
int is_file(int tar_fd, char *path) {
    int check = exists(tar_fd, path) - 10 ;
    return check == AREGTYPE || check == REGTYPE;
}

/**
 * Checks whether an entry exists in the archive and is a symlink.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 * @return zero if no entry at the given path exists in the archive or the entry is not symlink,
 *         any other value otherwise.
 */
int is_symlink(int tar_fd, char *path) {
    int check = exists(tar_fd, path) -10 ;
    printf("hello2");
    return check == SYMTYPE || check == LNKTYPE ;
}

/**
 * Lists the entries at a given path in the archive.
 * list() does not recurse into the directories listed at the given path.
 *
 * Example:
 *  dir/          list(..., "dir/", ...) lists "dir/a", "dir/b", "dir/c/" and "dir/e/"
 *   ├── a
 *   ├── b
 *   ├── c/
 *   │   └── d
 *   └── e/
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive. If the entry is a symlink, it must be resolved to its linked-to entry.
 * @param entries An array of char arrays, each one is long enough to contain a tar entry path.
 * @param no_entries An in-out argument.
 *                   The caller set it to the number of entries in `entries`.
 *                   The callee set it to the number of entries listed.
 *
 * @return zero if no directory at the given path exists in the archive,
 *         any other value otherwise.
 */

int list(int tar_fd, char *path, char **entries, size_t *no_entries) {
    *no_entries = 0;
    int check = exists(tar_fd, path) ;
    if (check == 0) return 0 ;

    if (check - 10 == SYMTYPE || check - 10 == LNKTYPE) { // if it is a link, getting his linked directory and calling list() on it !
        tar_header_t* header = (tar_header_t*) malloc(sizeof(tar_header_t)) ;
        lseek(tar_fd, -512, SEEK_CUR) ;
        read(tar_fd, header, 512) ;
        char* linkname = malloc(sizeof(char)*100) ;
        strcpy(linkname, header->linkname) ;
        linkname[strlen(linkname)] = '/' ;
        list(tar_fd, linkname, entries, no_entries) ; //going to link if it is a link
        free(linkname) ;
    }
    else if (check - 10 == DIRTYPE) { // travelling the whole directory if it is one
        tar_header_t* header = (tar_header_t*) malloc(sizeof(tar_header_t)) ;
        read(tar_fd, header, 512) ;
        int offset ;
        while (strstr(header->name, path) == header->name) { // running as long as the name of the directory stands in the file name
            // adding the name we found to entries & no_entries
            strcpy(entries[*no_entries], header->name) ;
            *no_entries += 1 ;
            if (header->typeflag == DIRTYPE) {// check if the found header is linked to a sub-directory
                // skipping full sub-directory because we have to not keep files it contains
                char* directory_name = malloc(sizeof(char)*100) ; // directory to skip
                strcpy(directory_name, header->name) ;
                lseek(tar_fd, 512, SEEK_CUR) ;
                read(tar_fd, header, 512) ;
                while (strstr(header->name, directory_name) == header->name) { // skipping all files if they are included in the directory to skip
                    offset = (TAR_INT(header->size)/512)*512;
                    if (TAR_INT(header->size)%512 != 0) offset += 512 ;
                    lseek(tar_fd, offset, SEEK_CUR) ;
                    read(tar_fd, header, 512) ;
                }
                free(directory_name) ;
            }
            else { // continuing to travel normally if header was not linked to a sub-directory
                offset = (TAR_INT(header->size)/512)*512;
                if (TAR_INT(header->size)%512 != 0) offset += 512 ;
                lseek(tar_fd, offset, SEEK_CUR) ;
                read(tar_fd, header, 512) ;
            }
        }
    }
    return 1;
}

/**
 * Reads a file at a given path in the archive.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive to read from.  If the entry is a symlink, it must be resolved to its linked-to entry.
 * @param offset An offset in the file from which to start reading from, zero indicates the start of the file.
 * @param dest A destination buffer to read the given file into.
 * @param len An in-out argument.
 *            The caller set it to the size of dest.
 *            The callee set it to the number of bytes written to dest.
 *
 * @return -1 if no entry at the given path exists in the archive or the entry is not a file,
 *         -2 if the offset is outside the file total length,
 *         zero if the file was read in its entirety into the destination buffer,
 *         a positive value if the file was partially read, representing the remaining bytes left to be read to reach
 *         the end of the file.
 *
 */
ssize_t read_file(int tar_fd, char *path, size_t offset, uint8_t *dest, size_t *len) {
    int check = exists(tar_fd, path) ;
    if (is_file(tar_fd, path) == 0 && check - 10 != SYMTYPE && check - 10 != LNKTYPE) return -1;

    tar_header_t* header = (tar_header_t*) malloc(sizeof(tar_header_t)) ;
    lseek(tar_fd, 0, SEEK_SET) ;
    read(tar_fd, header, 512) ;
    int oset = 0 ;
    int tot = 0;
    while (strcmp(path, header->name) != 0) {
        oset = (TAR_INT(header->size)/512)*512;
        if (TAR_INT(header->size)%512 != 0) oset += 512 ;
        tot += oset + 512;
        lseek(tar_fd, oset, SEEK_CUR) ;
        read(tar_fd, header, 512) ;
    }
    

    if (check - 10 == SYMTYPE || check - 10 == LNKTYPE) { // if it is a link, getting his linked directory and calling list() on it !
        ssize_t toReturn = read_file(tar_fd, header->linkname, offset, dest, len);
        return toReturn;
    }
    
    int s = TAR_INT(header->size) - offset;
    lseek(tar_fd, oset, SEEK_CUR) ;
    if (s < 0) return -2;
    
    if (s > *len) s = *len;
    
    *len = pread(tar_fd, dest, s,  tot + 512 + offset);
 
    ssize_t toReturn = TAR_INT(header->size) - offset - *len;
    
    free(header); 
    return toReturn;
}
