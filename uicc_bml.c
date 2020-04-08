#include <stdlib.h>
#include <stdio.h>

#include "uicc_bml.h"

// Header magic number, len = 14
static const uint8_t header_magic[] =
{0x00, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x53, 0x43, 0x42, 0x69, 0x6E};
static uint32_t ts_pointer_list_count = 0;
static struct ub_ts_pointer* ts_pointer_list[256];

static const struct ts_prop_type {
    uint8_t b1, b2, b3;
    const int len;                  // length of the data, in bytes
    const char* name;               // Friendly name
} ts_prop_type_data[] = {
    {0x01, 0x41, 0x2B, 1, "MenuGroup.Class"},
    {0x01, 0x00, 0x02, 4, "Referring Id <4>"},
    {0x01, 0x00, 0x03, 2, "Referring Id <2>"},
    {0x01, 0x00, 0x04, 1, "Referring Id <1>"},
    {0x01, 0x3D, 0x02, 4, "ApplicationModes <4>"},
    {0x01, 0x3D, 0x03, 2, "ApplicationModes <2>"},
    {0x01, 0x3D, 0x04, 1, "ApplicationModes <1>"},
    {0x01, 0x0B, 0x04, 1, "(01 01 0B 04) <1>"},
    {0x01, 0x0B, 0x09, 1, "(01 01 0B 09) <1>"},
    {0x01, 0x33, 0x04, 1, "(01 01 33 04) <1>"},
    {0x01, 0x46, 0x04, 1, "(01 01 46 04) <1>"},

    {0x04, 0x09, 0x00, 4, "(01 04 09 00) <4>"},
    {0x04, 0x0A, 0x00, 4, "(01 04 0A 00) <4>"},
    {0x04, 0x3F, 0x00, 4, "(01 04 3F 00) <4>"},
    {0x04, 0x44, 0x00, 4, "(01 04 44 00) <4>"},
    {0x04, 0x60, 0x00, 4, "(01 04 60 00) <4>"}
};

int ts_prop_guess_length(uint8_t b1, uint8_t b2, uint8_t b3) {
    if (b1 == 0x01) {
        if (b3 == 0x02)
            return 4;
        if (b3 == 0x03)
            return 2;
        if (b3 == 0x04)
            return 1;
        return 1;
    }
    else if (b3 == 0x04) {
        return 4;
    }
    return b1;
}

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
    {UBO_Gallery, "Gallery"},
    {UBO_MenuGroup, "MenuGroup"},
    {UBO_Tab, "Tab"},
    {UBO_QAT, "Quick Access Bar (Qat)"}
};

struct ts_prop_type* ub_ts_prop_type_from_bin(uint8_t b1, uint8_t b2, uint8_t b3) {
    for (int i = 0; i < sizeof(ts_prop_type_data) / sizeof(ts_prop_type_data[1]); i++) {
        if (ts_prop_type_data[i].b1 == b1 &&
            ts_prop_type_data[i].b2 == b2 &&
            ts_prop_type_data[i].b3 == b3)
            return ts_prop_type_data + i;
    }

    return NULL;
}

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

int ub_ts_prop_len(struct ub_ts_prop* prop) {
    struct ts_prop_type* type = ub_ts_prop_type_from_bin(prop->type_b1, prop->type_b2, prop->type_b3);
    return type == NULL ? prop->type_b1 : type->len;
}


const char* ub_ts_prop_name_str(struct ub_ts_prop* prop) {
    struct ts_prop_type* type = ub_ts_prop_type_from_bin(prop->type_b1, prop->type_b2, prop->type_b3);
    return type == NULL ? NULL : type->name;
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

const uint16_t* ub_ss_get(struct ub_ss* ss, uint16_t id) {
    for (int i = 0; i < ss->count; i++) {
        if (ss->strings[i]->id == id)
            return ss->strings[i]->wchars;
    }

    return L"INVALID_SS_ID";
}

void ub_free_ss(struct ub_ss* ss) {
    for (int i = 0; i < ss->count; i++) {
        if (ss->strings[i] != NULL)
            free(ss->strings[i]);
    }
    free(ss);
}

int ub_parse_ts_tag(FILE* hFile, void** ret) {
    *ret = 0;

    enum ub_ts_type tag_type= ub_byte(hFile);
    switch (tag_type) {
    case UB_TST_NODE:
        return ub_parse_ts_node(hFile, ret);
    case UB_TST_PROP:
        return ub_parse_ts_prop(hFile, ret);
    case UB_TST_COLLECTION:
        return ub_parse_ts_collection(hFile, ret);
    case UB_TST_POINTER:
        return ub_parse_ts_pointer(hFile, ret);
    case UB_TST_3B:
        return ub_parse_ts_3B(hFile, ret);
    }


    return UB_MSG_FAILED_UNKNOWN;
}

int ub_parse_ts_prop(FILE* hFile, struct ub_ts_prop** ret) {
    *ret = 0;
    long pos = ftell(hFile) - 1;

    uint8_t b1, b2, b3;
    b1 = ub_byte(hFile);
    b2 = ub_byte(hFile);
    b3 = ub_byte(hFile);
    struct ts_prop_type*  prop_type_dat = ub_ts_prop_type_from_bin(b1, b2, b3);
    if (prop_type_dat == NULL) {
        long pos = ftell(hFile);

        struct ts_prop_type prop_type_fake = {0, 0, 0, ts_prop_guess_length(b1, b2, b3), NULL};
        prop_type_dat = &prop_type_fake;
        printf("Warning: unknown property @0x%04X: (01 %02X %02X %02X) <%d> \n", pos, b1, b2, b3, prop_type_dat->len);
    }

    struct ub_ts_prop* prop = NULL;
    if (prop_type_dat->len <= 4) {
        prop = malloc(sizeof(struct ub_ts_prop));
        prop->data = 0;
        fread(&(prop->data), 1, prop_type_dat->len, hFile);
    }
    else {
        prop = malloc(sizeof(struct ub_ts_prop) + prop_type_dat->len);
        prop->data_ptr = prop + 1;
        fread(prop->data_ptr, 1, prop_type_dat->len, hFile);
    }

    prop->tag_type = UB_TST_PROP;
    prop->type_b1 = b1;
    prop->type_b2 = b2;
    prop->type_b3 = b3;
    //prop->fpos = pos;

    *ret = prop;
    return UB_OK;
}

int ub_parse_ts_node(FILE* hFile, struct ub_ts_node** ret) {
    *ret = 0;
    long pos = ftell(hFile) - 1;

    uint16_t type = ub_word(hFile);
    if (ub_word(hFile) != 0x1000)
        return UB_ERRMSG(UB_SRC_TS, UB_MSG_INVALID_FORMAT);
    uint16_t sizeInByte = ub_word(hFile);
    uint8_t childCount = ub_byte(hFile);

    struct ub_ts_node* node = malloc(sizeof(struct ub_ts_node) + (size_t)childCount*sizeof(void*));
    node->tag_type = UB_TST_NODE;
    node->type = type;
    node->length = sizeInByte;
    node->child_count = childCount;
    node->child_ptrs = node + 1;
    node->fpos = pos;

    for (int i = 0; i < childCount; i++) {
        void* child_ptr = NULL;
        long child_fpos = ftell(hFile);
        int r = ub_parse_ts_tag(hFile, &child_ptr);
        node->child_ptrs[i] = child_ptr;
    }

    *ret = node;
    return UB_OK;
}

int ub_parse_ts_collection(FILE* hFile, struct ub_ts_collection** ret) {
    *ret = 0;
    long pos = ftell(hFile)-1;

    if (ub_byte(hFile) != 0x01)
        return UB_ERRMSG(UB_SRC_TS, UB_MSG_INVALID_FORMAT);
    uint8_t type = ub_byte(hFile);
    uint16_t count = ub_word(hFile);

    struct ub_ts_collection* coll = malloc(sizeof(struct ub_ts_collection) + (size_t)count * sizeof(void*));
    coll->tag_type = UB_TST_COLLECTION;
    coll->type = type;
    coll->child_count = count;
    coll->child_ptrs = coll + 1;
    coll->fpos = pos;

    for (int i = 0; i < count; i++) {
        struct ub_ts_node* child_ptr = NULL;
        int r = ub_parse_ts_tag(hFile, &child_ptr);
        // long pos2 = ftell(hFile);
        coll->child_ptrs[i] = child_ptr;
    }

    *ret = coll;
    return UB_OK;
}

int ub_parse_ts_pointer(FILE* hFile, struct ub_ts_pointer** ret) {
    //long pos = ftell(hFile) - 1;

    struct ub_ts_pointer* node = malloc(sizeof(struct ub_ts_pointer));
    node->tag_type = UB_TST_POINTER;
    node->target_addr = ub_dword(hFile);
    //node->fpos = pos;

    ts_pointer_list[ts_pointer_list_count] = node;
    ts_pointer_list_count++;

    *ret = node;
    return UB_OK;
}

uint32_t ub_ts_pointer_count() {
    return ts_pointer_list_count;
}

struct ub_ts_pointer* ub_ts_pointer_get(uint32_t id) {
    return ts_pointer_list[id];
}

int ub_parse_ts_3B(FILE* hFile, struct ub_ts_3B** ret) {
    //long pos = ftell(hFile) - 1;

    *ret = malloc(sizeof(struct ub_ts_3B));
    (*ret)->tag_type = UB_TST_3B;
    (*ret)->type = ub_byte(hFile);
    //(*ret)->fpos = pos;

    if ((*ret)->type == 0x09)
        (*ret)->data = ub_byte(hFile);
    else if ((*ret)->type == 0x03)
        (*ret)->data = ub_word(hFile);
    else if ((*ret)->type == 0x02)
        (*ret)->data = ub_dword(hFile);
    else 
        return UB_ERRMSG(UB_SRC_TS, UB_MSG_INVALID_FORMAT);

    return UB_OK;
}