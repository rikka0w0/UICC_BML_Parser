#include <stdlib.h>
#include <stdio.h>

#include "uicc_bml.h"




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
    ub_free_uss(uss);


    struct ub_ac* ac;
    r = ub_parse_ac(hFile, &ac);
    printf("# Parsing the Application.Command section\n");
    printf("count = %d\n", ac->count);
    for (int i = 0; i < ac->count; i++) {
        printf("%d. Id = 0x%04X (%d)\n", i + 1, ac->tags[i]->id, ac->tags[i]->id);
        for (int j = 0; j < ac->tags[i]->count; j++) {
            struct ub_ac_pair* prop = ac->tags[i]->properties[j];
            printf("   0x%02X = 0x%X (%s = %d) \n", (int)prop->type, prop->value, ub_prop_type_str(prop->type), prop->value);
        }
    }
    printf("\n");
    ub_free_ac(ac);

    struct ub_ss* ss;
    r = ub_parse_ss(hFile, &ss);
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
    ub_free_ss(ss);

    if (ub_byte(hFile) != 0x0D)
        return UB_ERRMSG(UB_SRC_TS, UB_MSG_INVALID_FORMAT);
    if (ub_word(hFile) != 0x0003)
        return UB_ERRMSG(UB_SRC_TS, UB_MSG_INVALID_FORMAT);
    uint32_t ps_offset = ub_dword(hFile);

    uint32_t sz = ps_offset - ftell(hFile);
    uint8_t* mem = malloc(sz);
    fread(mem, 1, sz, hFile);

    FILE* bin = fopen("tree.bin", "wb");
    fwrite(mem, 1, sz, bin);
    fclose(bin);

    free(mem);

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

    printf("Hello World!\n");

    exit(EXIT_SUCCESS);
}

// "D:\Administrator\Desktop\Win32Ribbon\WORDPAD_RIBBON.bin"
// "D:\Administrator\Desktop\Win32Ribbon\Win32Ribbon\ribbon.bml"

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
