#pragma once
#ifndef _INC_UICC_BML // include guard for 3rd party interop
#define _INC_UICC_BML

#include <stdint.h>

struct ub_uss {
	uint32_t length;
	uint8_t count;
	char** strings;
};

struct ub_ac {	// Application.Commands
	uint32_t count;
	struct ub_ac_tag**tags;
};

struct ub_ac_tag {
	uint16_t id;
	uint8_t count;
	struct ub_ac_pair**properties;
};

struct ub_ac_pair {
	enum ub_ac_property_type type;	// Byte
	uint32_t value;
	uint32_t auxdata;
};

enum ub_ac_property_type {
	Command_LabelTitle = 1,
	Command_SmallImages = 5,
	Command_LargeImages = 6
};

enum ub_object_type {
	UBO_ToggleButton = 0x0600,
	UBO_Group = 0x0700,
	UBO_Button = 0x0F00,
	UBO_FileMenu = 0x1300,
	UBO_Gallery = 0x1500,
	UBO_MenuGroup = 0x1800,
	UBO_Tab = 0x1A00,
	UBO_QAT = 0x2500
};

struct ub_ss {
	uint32_t length;
	uint32_t count;
	struct ub_ss_string** strings;
};

struct ub_ss_string {
	uint16_t id;
	enum ub_object_type type;	// WORD
	uint16_t length;
	uint16_t* wchars;
};

enum ub_ts_type {
	UB_TST_PROP = 0x01,
	UB_TST_NODE = 0x16,
	UB_TST_COLLECTION = 0x18,
	UB_TST_POINTER = 0x3E,
	UB_TST_3B = 0x3B
};

struct ub_ts_prop {
	enum ub_ts_type tag_type;	// BYTE
	uint8_t type_b1, type_b2, type_b3;
	union payload
	{
		uint32_t data;
		uint8_t* data_ptr;
	};
};

struct ub_ts_node {
	enum ub_ts_type tag_type;	// BYTE
	enum ub_object_type type;	// WORD
	uint16_t length;			// Length of the node in bytes
	uint16_t child_count;		// Number of child nodes
	uint32_t fpos;
	void** child_ptrs;
};

struct ub_ts_collection {
	enum ub_ts_type tag_type;	// BYTE
	uint8_t type;
	uint16_t child_count;		// Number of child nodes
	uint32_t fpos;
	void** child_ptrs;
};

struct ub_ts_pointer {
	enum ub_ts_type tag_type;	// BYTE
	uint32_t target_addr;
	struct ub_ts_collection* target_coll;
};

struct ub_ts_3B {
	enum ub_ts_type tag_type;	// BYTE
	uint8_t type;
	uint32_t data;
};

enum ub_src {
	UB_SRC_UNKNOWN,
	UB_SRC_FILE,
	UB_SRC_USS,
	UB_SRC_AC,
	UB_SRC_SS,
	UB_SRC_TS,
	UB_SRC_PS,

	ub_src_len	// Do not use
};

enum ub_err {
	UB_MSG_OK,
	UB_MSG_INVALID_HEADER,
	UB_MSG_INVALID_FORMAT,
	UB_MSG_INVALID_LENGTH,
	UB_MSG_FAILED_UNKNOWN,

	ub_msg_len	// Do not use
};

#define UB_OK UB_ERRMSG(UB_SRC_UNKNOWN, UB_MSG_OK)
#define UB_FAILED UB_ERRMSG(UB_SRC_UNKNOWN, UB_MSG_FAILED_UNKNOWN)
#define UB_ERRMSG(src, msg) (((src&0xFF)<<24) | (msg&0xFFFF))
#define UB_ERRMSG_SRC(errmsg) ((enum ub_src) ((errmsg>>24)&0xFF))
#define UB_ERRMSG_MSG(errmsg) ((enum ub_err) (msg&0xFFFF))


// fp stands for file pointer

// Check the header magic number and fp+=14
int ub_check_header(FILE* file);

// Read a BYTE/WORD/DWORD and fp+=1/2/4
uint8_t ub_byte(FILE* hFile);
uint16_t ub_word(FILE* hFile);
uint32_t ub_dword(FILE* hFile);

int ub_parse_uss(FILE* hFile, struct ub_uss** ret);
void ub_free_uss(struct ub_uss* uss);

int ub_parse_ac(FILE* hFile, struct ub_ac** ret);
char* ub_prop_type_str(enum ub_ac_property_type type);
void ub_free_ac(struct ub_ac* ac);

int ub_parse_ss(FILE* hFile, struct ub_ss** ret);
const uint16_t* ub_ss_get(struct ub_ss* ss, uint16_t id);
const char* ub_obj_type_str(enum ub_object_type type);
void ub_free_ss(struct ub_ss* ss);

int ub_parse_ts_tag(FILE* hFile, void** ret);
int ub_parse_ts_prop(FILE* hFile, struct ub_ts_prop** ret);
int ub_parse_ts_node(FILE* hFile, struct ub_ts_node** ret);
int ub_parse_ts_collection(FILE* hFile, struct ub_ts_collection** ret);
int ub_parse_ts_pointer(FILE* hFile, struct ub_ts_pointer** ret);
int ub_parse_ts_3B(FILE* hFile, struct ub_ts_3B** ret);

int ub_ts_prop_len(struct ub_ts_prop*);
const char* ub_ts_prop_name_str(struct ub_ts_prop*);
#endif