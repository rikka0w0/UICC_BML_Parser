#include <stdlib.h>
#include <stdio.h>

#include "uicc_bml.h"

// Header magic number, len = 14
static const uint8_t header_magic[] =
{0x00, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x53, 0x43, 0x42, 0x69, 0x6E};

static const struct ac_prop_data_item {
    enum ub_ac_property_type type;  // 1 byte
    const int len;                  // length of the data
    const char* name;               // Friendly name
} ac_prop_data[] = {
    {0x01, 4, "Command.LabelTitle"},
    {0x02, 4, "Command.LabelDescription"},
    {0x03, 6, "Command.SmallHighContrastImages"},
    {0x04, 6, "Command.LargeHighContrastImages"},
    {0x05, 6, "Command.SmallImages"},
    {0x06, 6, "Command.LargeImages"},
    {0x07, 4, "Command.Keytip"},
    {0x08, 4, "Command.TooltipTitle"},
    {0x09, 4, "Command.TooltipDescription"}
};

static const struct object_type_name_pair {
    enum ub_object_type type;
    const char* name;
} object_type_name_mapping[] = {
    {UBO_ToggleButton, "Toggle Button"},
    {UBO_Group, "Group"},
    {UBO_Button, "Button"},
    {UBO_FileMenu, "\"File\" Menu"},
    {UBO_MenuGroup, "MenuGroup"},
    {UBO_Tab, "Tab"},
    {UBO_QAT, "Quick Access Bar (Qat)"}
};

struct ac_prop_data_item* ub_prop_data_from_type(enum ub_ac_property_type type) {
    for (int i = 0; i < sizeof(ac_prop_data) / sizeof(ac_prop_data[1]); i++) {
        if (ac_prop_data[i].type == type)
            return ac_prop_data + i;
    }

    return NULL;
}

char* ub_prop_type_str(enum ub_ac_property_type type) {
    struct ac_prop_data_item* pdi = ub_prop_data_from_type(type);
    return pdi == NULL ? "Unknown Type" : pdi->name;
}

const char* ub_obj_type_str(enum ub_object_type type) {
    for (int i = 0; i < sizeof(object_type_name_mapping) / sizeof(object_type_name_mapping[1]); i++) {
        if (object_type_name_mapping[i].type == type)
            return object_type_name_mapping[i].name;
    }
    return "Unknown Type";
}

int ub_check_header(FILE* hFile) {
	uint8_t buf[sizeof(header_magic)];
	int rlen = fread(buf, 1, sizeof(header_magic), hFile);
	if (rlen != sizeof(header_magic))
		return 0;

	
	for (int i = 0; i < sizeof(header_magic); i++) {
		if (buf[i] != header_magic[i])
			return 0;
	}
	return 1;
}

uint8_t ub_byte(FILE* hFile) {
	uint8_t buf;
	fread(&buf, 1, sizeof(uint8_t), hFile);
	return buf;
}

uint16_t ub_word(FILE* hFile) {
	uint16_t buf;
	fread(&buf, 1, sizeof(uint16_t), hFile);
	return buf;
}

uint32_t ub_dword(FILE* hFile) {
	uint32_t buf;
	fread(&buf, 1, sizeof(uint32_t), hFile);
	return buf;
}


int ub_parse_uss(FILE* hFile, struct ub_uss** ret) {
    long startPos = ftell(hFile);

    *ret = 0;

    if (ub_byte(hFile) != 0x02)
        return UB_ERRMSG(UB_SRC_USS, UB_MSG_INVALID_HEADER);
    uint32_t length = ub_dword(hFile);
    if (ub_byte(hFile) != 0x01)
        return UB_ERRMSG(UB_SRC_USS, UB_MSG_INVALID_FORMAT);
    uint8_t count = ub_byte(hFile);

    struct ub_uss* uss = malloc(sizeof(struct ub_uss) + sizeof(char*)*count);
    uss->length = length;
    uss->count = count;
    uss->strings = uss + 1;

    for (int i = 0; i < uss->count; i++) {
        if (ub_byte(hFile) != 0x01)
            return UB_ERRMSG(UB_SRC_USS, UB_MSG_INVALID_FORMAT);

        uint16_t lenStr = ub_word(hFile);

        uss->strings[i] = malloc(lenStr + (size_t)1);
        fread(uss->strings[i], 1, lenStr, hFile);
        uss->strings[i][lenStr] = 0; // Add the terminator
    }

    if (ftell(hFile) != startPos + uss->length)
        return UB_ERRMSG(UB_SRC_USS, UB_MSG_INVALID_LENGTH);

    *ret = uss;
    return UB_OK;
}

void ub_free_uss(struct ub_uss* uss) {
    for (int i=0; i<uss->count; i++)
        free(uss->strings[i]);
    free(uss);
}

int ub_parse_ac(FILE* hFile, struct ub_ac** ret) {
    *ret = 0;

    if (ub_byte(hFile) != 0x0F)
        return UB_ERRMSG(UB_SRC_AC, UB_MSG_INVALID_HEADER);
    uint32_t count = ub_dword(hFile);

    struct ub_ac* ac = malloc(sizeof(struct ub_ac) + count * sizeof(struct ub_ac_tag*));
    ac->count = count;
    ac->tags = (struct ub_ac_tag**) (ac + 1);

    for (int i = 0; i < count; i++) {
        uint16_t id = ub_word(hFile);
        if (ub_word(hFile) != 0x0000)
            return UB_ERRMSG(UB_SRC_AC, UB_MSG_INVALID_FORMAT);
        uint8_t count = ub_byte(hFile);

        struct ub_ac_tag* tag = malloc(sizeof(struct ub_ac_tag) + sizeof(struct ub_ac_pair*) * count);
        tag->id = id;
        tag->count = count;
        tag->properties = tag + 1;
        ac->tags[i] = tag;


        for (int j = 0; j < ac->tags[i]->count; j++) {
            struct ub_ac_pair* prop = malloc(sizeof(struct ub_ac_pair));
            prop->type = ub_byte(hFile);
            prop->value = ub_dword(hFile);
            prop->auxdata = 0;
            ac->tags[i]->properties[j] = prop;

            struct ac_prop_data_item* pdi = ub_prop_data_from_type(ac->tags[i]->properties[j]->type);
            int auxlen = (pdi==NULL) ? 0 : pdi->len-4;
            if (auxlen > 0) {
                uint32_t auxdata = 0;
                fread(&auxdata, 1, auxlen > 4 ? 4 : auxlen, hFile);
                ac->tags[i]->properties[j]->auxdata = auxdata;
            }
        }
    }

    *ret = ac;
    return UB_OK;
}

void ub_free_ac(struct ub_ac* ac) {
    for (int i = 0; i < ac->count; i++) {
        for (int j = 0; j < ac->tags[i]->count; j++) {
            free(ac->tags[i]->properties[j]);
        }
        free(ac->tags[i]);
    }
    free(ac);
}

int ub_parse_ss(FILE* hFile, struct ub_ss** ret) {
    *ret = 0;

    if (ub_byte(hFile) != 0x10)
        return UB_ERRMSG(UB_SRC_SS, UB_MSG_INVALID_HEADER);

    uint32_t length = ub_dword(hFile);
    uint32_t count = ub_dword(hFile);
    struct ub_ss* ss = (struct ub_ss*)malloc(sizeof(struct ub_ss) + count * sizeof(struct ub_ss_string*));
    ss->length = length;
    ss->count = count;
    ss->strings = (struct ub_ss_string*) (ss + 1);

    for (int i = 0; i < ss->count; i++) {
        long pos = ftell(hFile);
        uint16_t id = ub_word(hFile);
        if (ub_word(hFile) != 0x0000)
            return UB_ERRMSG(UB_SRC_SS, UB_MSG_INVALID_FORMAT);
        uint16_t obj_type = ub_word(hFile);
        uint16_t magic = ub_word(hFile);
        if (magic != 0x1000)
           ;//return UB_ERRMSG(UB_SRC_SS, UB_MSG_INVALID_FORMAT);
        uint16_t lenwchar = ub_word(hFile);

        struct ub_ss_string* ss_string = malloc(sizeof(struct ub_ss_string) + lenwchar + 2);
        ss->strings[i] = ss_string;
        ss_string->id = id;
        ss_string->type = obj_type;
        ss_string->length = lenwchar;
        ss_string->wchars = (uint16_t*)(ss_string + 1);

        fread(ss_string->wchars, 1, lenwchar, hFile);
        ss_string->wchars[lenwchar >> 1] = 0x0000;
    }


    *ret = ss;
    return UB_OK;
}

void ub_free_ss(struct ub_ss* ss) {
    for (int i = 0; i < ss->count; i++) {
        if (ss->strings[i] != NULL)
            free(ss->strings[i]);
    }
    free(ss);
}