#include "../lib/operations.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int find_child_inode(file_system *fs, int parent_inode, const char *name);
int find_inode_by_path(file_system *fs, const char *path) {
    //Function implementation to find inode by path
    char path_copy[NAME_MAX_LENGTH];
    strncpy(path_copy, path, NAME_MAX_LENGTH);
    
    char *token = strtok(path_copy, "/");
    long int current_inode = fs->root_node;
    
    while (token != NULL) {
        int child_inode_index = find_child_inode(fs, current_inode, token);
        
        if (child_inode_index == -1) {
            return -1; 
        }
        
        current_inode = child_inode_index;
        
        token = strtok(NULL, "/");
    }
    
    return current_inode;
}



int find_child_inode(file_system *fs, int parent_inode, const char *name)
//Function implementation to find a child inode within a parent inode
{
    for (int i = 0; i < DIRECT_BLOCKS_COUNT; ++i) {
        int child_inode_index = fs->inodes[parent_inode].direct_blocks[i];
        
        if (child_inode_index != -1) {
            inode *child_inode = &(fs->inodes[child_inode_index]);
            
            if (strcmp(child_inode->name, name) == 0) {
                return child_inode_index;
            }
        }
    }
    
    return -1;
}


//Create a new directory at the specified path in the file system
int fs_mkdir(file_system *fs, char *path) {
    int inode_ind = find_free_inode(fs);
    if (inode_ind == -1) {
        return -1;
    }
    if (path == 0 || path[0] != '/') {
        return -1;
    }

    inode *new_inode = &(fs->inodes[inode_ind]);
    inode_init(new_inode);
    new_inode->n_type = directory;

    char *root_path = strdup(path);
    char *pres = strrchr(root_path, '/');
    if (pres != NULL) {
    *pres = '\0';
    int name_length = strlen(pres + 1);
    if (name_length < sizeof(new_inode->name)) {
        strncpy(new_inode->name, pres + 1, name_length + 1);
    } else {
        strncpy(new_inode->name, pres + 1, sizeof(new_inode->name));
    }
    } else {
    int path_length = strlen(path);
    if (path_length < sizeof(new_inode->name)) {
        strncpy(new_inode->name, path, path_length + 1);
    } else {
        strncpy(new_inode->name, path, sizeof(new_inode->name));
    }
}

    int parent_inode_index = -1;
    int root_path1 = 0;
    if (pres != NULL) {
        parent_inode_index = find_inode_by_path(fs, root_path);
        if (parent_inode_index != -1) {
            root_path1 = 1;
        }
    }

    if (!root_path1) {
        inode_init(new_inode);
        free(root_path);
        return -1;
    }

    inode *parent_inode = &(fs->inodes[parent_inode_index]);
    int block_index = 0;
    int found_empty_block = 0;
    while (block_index < DIRECT_BLOCKS_COUNT && !found_empty_block) {
        if (parent_inode->direct_blocks[block_index] == -1) {
            parent_inode->direct_blocks[block_index] = inode_ind;
            found_empty_block = 1;
        }
        block_index++;
    }

    new_inode->parent = parent_inode_index;

    free(root_path);
    return 0;
}


//Create a new file at the specified path in the file system
int fs_mkfile(file_system *fs, char *pathx) {
    int inode_ind = find_free_inode(fs);
    if (inode_ind == -1) {
        return -1;
    }
    if (pathx[0] != '/') {
        return -1;
    }

    inode *new_file_inode = &(fs->inodes[inode_ind]);
    inode_init(new_file_inode);
    new_file_inode->n_type = reg_file;

    char *root_path = strdup(pathx);
    char *pres = strrchr(root_path, '/');
    if (pres != NULL) {
        *pres = '\0';
        strcpy(new_file_inode->name, pres+1);
    } else {
        strcpy(new_file_inode->name, pathx);
    }

    int parent_inode_index = -1;
    int should_return = 0;

    if (pres != NULL) {
        parent_inode_index = find_inode_by_path(fs, root_path);
        if (parent_inode_index == -1) {
            should_return = 1;
        } else {
            inode *parent_inode = &(fs->inodes[parent_inode_index]);
            int block_index = 0;
            int found_empty_block = 0;
            while (block_index < DIRECT_BLOCKS_COUNT && !found_empty_block) {
                if (parent_inode->direct_blocks[block_index] == -1) {
                    parent_inode->direct_blocks[block_index] = inode_ind;
                    found_empty_block = 1;
                }
                block_index++;
            }

            new_file_inode->parent = parent_inode_index;
        }
    } else {
        inode *parent_inode = &(fs->inodes[parent_inode_index]);
        for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
            if (parent_inode->direct_blocks[i] != -1) {
                should_return = 1;
                break;
            }
        }

        if (!should_return) {
            for (int i = 0; i < DIRECT_BLOCKS_COUNT; ++i) {
                if (parent_inode->direct_blocks[i] == -1) {
                    parent_inode->direct_blocks[i] = inode_ind;
                    break;
                }
            }

            new_file_inode->parent = parent_inode_index;
        }
    }

    free(root_path);

    if (should_return) {
        inode_init(new_file_inode);
        return -1;
    }

    return 0;
}


//List all files and directories within the specified directory path
char *fs_list(file_system *fs, char *path) {
    int inode_index = find_inode_by_path(fs, path);
    if (inode_index == -1) {
        return NULL;
    }
        if (path[0] != '/' || path == NULL) {
        return NULL;
    }

    inode *current_inode = &(fs->inodes[inode_index]);

    int total_entries = 0;
    for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
        if (current_inode->direct_blocks[i] != -1) {
            total_entries++;
        }
    }

    char *result = (char *)malloc(total_entries * (NAME_MAX_LENGTH + 5) + 1);
    if (!result) {
        return NULL;
    }
    result[0] = '\0';

    if (current_inode->n_type == directory) {
        for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
            int child_inode_index = current_inode->direct_blocks[i];
            if (child_inode_index != -1) {
                inode *child_inode = &(fs->inodes[child_inode_index]);
                char *child_name = child_inode->name;
                const char *entry_type = (child_inode->n_type == directory) ? "DIR" : "FIL";
                sprintf(result + strlen(result), "%s %s\n", entry_type, child_name);
            }
        }
    }

    return result;
}


//Write text data to the specified file within the file system
int fs_writef(file_system *fs, char *filename, char *text) {
    if (fs == NULL || filename == NULL || text == NULL) {
        return -1;
    }

    int file_inode_index = find_inode_by_path(fs, filename);
    if (file_inode_index == -1) {
        return -1;
    }

    inode *file_inode = &(fs->inodes[file_inode_index]);
    int remaining_text_length = strlen(text);
    int total_bytes_written = 0;

    for (int i = 0; i < DIRECT_BLOCKS_COUNT; ++i) {
        int block_index = file_inode->direct_blocks[i];

        if (block_index == -1) {
            int free_block_index = -1;
            for (int j = 0; j < fs->s_block->num_blocks; ++j) {
                if (fs->free_list[j] == 1) {
                    fs->free_list[j] = 0;
                    fs->s_block->free_blocks--;
                    free_block_index = j;
                    file_inode->direct_blocks[i] = free_block_index;
                    break;
                }
            }

            if (free_block_index == -1) {
                return -1;
            }

            block_index = free_block_index;
        }

        data_block *block = &(fs->data_blocks[block_index]);
        int remaining_space = BLOCK_SIZE - block->size;
        if (remaining_space == 0) {
            continue; 
        }

        int write_length = remaining_space < remaining_text_length ? remaining_space : remaining_text_length;

        strncpy((char*)block->block + block->size, text, write_length);
        block->size += write_length;

        total_bytes_written += write_length;
        text += write_length;
        remaining_text_length -= write_length;

        if (remaining_text_length <= 0) {
            break;
        }
    }

    return total_bytes_written;
}


//Read data from the specified file and return it as a dynamically allocated buffer
uint8_t *fs_readf(file_system *fs, char *filename, int *file_size) {
    if (fs == NULL || filename == NULL || file_size == NULL) {
        return NULL;
    }

    int file_inode_index = find_inode_by_path(fs, filename);
    if (file_inode_index == -1) {
        return NULL;
    }

    inode *file_inode = &(fs->inodes[file_inode_index]);

    if (file_inode->size <= 0) {
        return NULL;
    }

    *file_size = file_inode->size;
    uint8_t *file_data = (uint8_t*)malloc(file_inode->size + 1);
    if (file_data == NULL) {
        return NULL;
    }

    file_data[file_inode->size] = '\0';

    int position = 0;
    for (int i = 0; i < DIRECT_BLOCKS_COUNT; ++i) {
        int block_index = file_inode->direct_blocks[i];
        if (block_index != -1) {
            data_block *block = &(fs->data_blocks[block_index]);
            memcpy(file_data + position, block->block, block->size);
            position += block->size;
        }
    }

    return file_data;
}


//Removes a file or directory at the specified path from the file system
int fs_rm(file_system *fs, char *path) {
    if (path == NULL || path[0] != '/') {
        //printf("Invalid path format\n");
        return -1;
    }

    int inode_index = find_inode_by_path(fs, path);
    if (inode_index == -1) {
        return -1;
    }

    inode *current_inode = &(fs->inodes[inode_index]);

    if (current_inode->n_type == directory) {
        int i = 0;
        while (i < DIRECT_BLOCKS_COUNT) {
            int sub_block_index = current_inode->direct_blocks[i];
            if (sub_block_index != -1) {
                char sub_path[NAME_MAX_LENGTH];
                snprintf(sub_path, sizeof(sub_path), "%s/%s", path, fs->inodes[sub_block_index].name);
                int removal_result = fs_rm(fs, sub_path);
                if (removal_result == -1) {
                    return -1;
                }
            }
            i++;
        }
    }

    int j = 0;
    while (j < DIRECT_BLOCKS_COUNT) {
        int direct_block_index = current_inode->direct_blocks[j];
        if (direct_block_index != -1) {
            fs->free_list[direct_block_index] = 1;
            current_inode->direct_blocks[j] = -1;
        }
        j++;
    }

    inode *parent_inode = &(fs->inodes[current_inode->parent]);

    int found_direct_block = 0;
    for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
        if (parent_inode->direct_blocks[i] == current_inode - fs->inodes) {
            parent_inode->direct_blocks[i] = -1;
            found_direct_block = 1;
            break;
        }
    }

    if (!found_direct_block) {
        return -1;
    }

    for (int i = 0; i < DIRECT_BLOCKS_COUNT; i++) {
        int direct_block_index = current_inode->direct_blocks[i];
        if (direct_block_index != -1) {
            fs->free_list[direct_block_index] = 1;
            current_inode->direct_blocks[i] = -1;
        }
    }

    current_inode->n_type = free_block;
    memset(current_inode->name, 0, NAME_MAX_LENGTH);
    memset(current_inode->direct_blocks, -1, DIRECT_BLOCKS_COUNT * sizeof(int));
    current_inode->parent = -1;

    return 0;
}




int import_file_data(file_system *fs, char *internal_file_path, char *file_data);
//Import data from an external file into the file system
int fs_import(file_system *fs, char *internal_file_path, char *external_file_path) {
    if (!fs || !internal_file_path || !external_file_path) {
        return -1;
    }

    FILE *external_file = fopen(external_file_path, "rb");
    if (!external_file) {
        if (errno == ENOENT) {
            return -2;
        } else {
            return -3;
        }
    }

    fseek(external_file, 0, SEEK_END);
    long file_size = ftell(external_file);
    fseek(external_file, 0, SEEK_SET);

    if (file_size <= 0) {
        fclose(external_file);
        return 0;
    }

    char *file_data = (char *)malloc(file_size + 1);
    if (!file_data) {
        fclose(external_file);
        return -4;
    }

    size_t bytes_read = fread(file_data, 1, file_size, external_file);
    if (bytes_read != (size_t)file_size) {
        fclose(external_file);
        free(file_data);
        return -5;
    }
    file_data[file_size] = '\0';
    fclose(external_file);

    int result = import_file_data(fs, internal_file_path, file_data);
    free(file_data);

    if (result == -1) {
        return -6;
    }

    return 0;
}


//Write the imported file data into the file system
int import_file_data(file_system *fs, char *internal_file_path, char *file_data) {
    int write_result = fs_writef(fs, internal_file_path, file_data);
    if (write_result == -1) {
        return -1;
    }

    return 0;
}



//Check if a file exists at the specified path
int file_exists(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}


//Export data from the file system to an external file
int fs_export(file_system *fs, char *internal_file_path, char *external_file_path) {
    if (!fs || !internal_file_path || !external_file_path) {
        return -1;
    }

    if (file_exists(external_file_path)) {
        return -1;
    }
    int file_size;
    uint8_t *data = fs_readf(fs, internal_file_path, &file_size);
    if (!data || file_size <= 0) {
        return -1;
    }

    FILE *file = fopen(external_file_path, "wb");
    if (!file) {
        return -1;
    }

    fwrite(data, sizeof(uint8_t), file_size, file);
    fclose(file);

    return 0;
}
