#include <stdlib.h>
#include <stdio.h>

#include "uicc_bml.h"

struct ub_ss* g_ss = NULL;

#define printl(start, level, ...) { \
for (int i = 0; i < (start?level-1:level); i++) printf("|  "); \
if (start && level>0) printf("|--"); \
printf(__VA_ARGS__); \
}

void print_ts_node(struct ub_ts_node* node, int level) {
    printl(1, level, "Node: 0x%04X (%s), %d children\n", node->type, ub_obj_type_str(node->type), node->child_count);
    printl(0, level, "{\n");
    level++;

    for (int i = 0; i < node->child_count; i++) {
        enum ub_ts_type tag_type = *((enum ub_ts_type*) node->child_ptrs[i]);
        if (tag_type == UB_TST_PROP) {
            struct ub_ts_prop* prop = node->child_ptrs[i];
            printl(1, level, "Prop: %s =", ub_ts_prop_name_str(prop));
            int len = ub_ts_prop_len(prop);
            if (len > 4) {
                for (int j = 0; j < len; j++) {
                    printf(" %02X", prop->data_ptr[j]);
                }
                printf("\n");
            }
            else if (prop->type_b1 == 0x01 && prop->type_b2 == 0x00 && prop->type_b3 == 0x03) {
                printf(" 0x%08X (%d) \033[0;31m%S\033[0m\n", prop->data, prop->data, ub_ss_get(g_ss, prop->data));
            }
            else {
                printf(" 0x%08X (%d)\n", prop->data, prop->data);
            }
        }
        else if (tag_type == UB_TST_NODE) {
            struct ub_ts_node* childnode = node->child_ptrs[i];
            print_ts_node(childnode, level);
        }
        else if (tag_type == UB_TST_COLLECTION) {
            struct ub_ts_collection* coll = node->child_ptrs[i];
            printl(1, level, "Collection: type = 0x%02X (%d), %d children\n", coll->type, coll->type, coll->child_count);
            
            printl(0, level, "[\n");
            level++;
            for (int j = 0; j < coll->child_count; j++) {
                print_ts_node(coll->child_ptrs[j], level);
            }
            level--;
            printl(0, level, "]\n");
        }
        else if (tag_type == UB_TST_POINTER) {
            struct ub_ts_pointer* pointer = node->child_ptrs[i];
            printl(1, level, "Pointer: -> 0x%08X\n", pointer->target_addr);
        }
    }

    level--;
    printl(0, level, "}\n");
}

void parse(FILE* hFile) {
    int header_valid = ub_check_header(hFile);
    if (!header_valid)
        printf("FILE - Invalid header!");

    uint32_t lenFile = ub_dword(hFile);
    printf("Size of the file: %d \n", lenFile);
    printf("\n");


    struct ub_uss* uss;
    int r = ub_parse_uss(hFile, &uss);
    printf("# Parsing the Unknown String section\n");
    printf("length = %d\n", uss->length);
    printf("count = %d\n", uss->count);
    for (int i = 0; i < uss->count; i++) {
        printf("%d = %s\n", i+1, uss->strings[i]);
    }
    printf("\n");


    struct ub_ac* ac;
    r = ub_parse_ac(hFile, &ac);
    struct ub_ss* ss;
    r = ub_parse_ss(hFile, &ss);
    g_ss = ss;

    printf("# Parsing the Application.Command section\n");
    printf("count = %d\n", ac->count);
    for (int i = 0; i < ac->count; i++) {
        printf("%d. Id = 0x%04X (%d)  \033[0;31m%S\033[0m\n", i + 1, ac->tags[i]->id, ac->tags[i]->id, ub_ss_get(g_ss, ac->tags[i]->id));
        for (int j = 0; j < ac->tags[i]->count; j++) {
            struct ub_ac_pair* prop = ac->tags[i]->properties[j];
            printf("   0x%02X = 0x%X (%s = %d) \n", (int)prop->type, prop->value, ub_prop_type_str(prop->type), prop->value);
        }
    }
    printf("\n");


    printf("# Parsing the String section\n");
    printf("length = %d\n", ss->length);
    printf("count = %d\n", ss->count);
    for (int i = 0; i < ss->count; i++) {
        struct ub_ss_string* string = ss->strings[i];
        printf("%d. Id = 0x%04X (%d)\n", i + 1, string->id, string->id);
        printf("   type = 0x%04X (%s) \n", string->type, ub_obj_type_str(string->type));
        printf("   %S \n", string->wchars);
    }
    printf("\n");

    if (ub_byte(hFile) != 0x0D)
        return UB_ERRMSG(UB_SRC_TS, UB_MSG_INVALID_FORMAT);
    if (ub_word(hFile) != 0x0003)
        return UB_ERRMSG(UB_SRC_TS, UB_MSG_INVALID_FORMAT);
    uint32_t ps_offset = ub_dword(hFile);

    struct ub_ts_node* rootNode;
    r= ub_parse_ts_tag(hFile, &rootNode);
    printf("# Parsing the Tree section\n");
    print_ts_node(rootNode, 0);
    //printf("Tree Section starts at 0x%04X\n", ftell(hFile));
    //uint32_t sz = ps_offset - ftell(hFile);
    //uint8_t* mem = malloc(sz);
    //fread(mem, 1, sz, hFile);

    //FILE* bin = fopen("tree.bin", "wb");
    //fwrite(mem, 1, sz, bin);
    //fclose(bin);

    //free(mem);

    ub_free_uss(uss);
    ub_free_ac(ac);
    ub_free_ss(ss);

    return;
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        printf("Need input file!\n");
        exit(EXIT_FAILURE);
    }

    char* filename = argv[1];
    FILE* hFile = fopen(filename, "rb");
    if (hFile == NULL) {
        printf("Failed to open the UICC bml file!\n");
        exit(EXIT_FAILURE);
    }

    parse(hFile);
    fclose(hFile);

    printf("Done!\n");

    exit(EXIT_SUCCESS);
}

// "D:\Administrator\Desktop\Win32Ribbon\WORDPAD_RIBBON.bin"
// "D:\Administrator\Desktop\Win32Ribbon\Win32Ribbon\ribbon.bml"