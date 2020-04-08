# Introduction
The `UICC.exe` produces a binary description file from the source XAML, this binary is embedded in the generated RC resource file.
The format of the binary file is not documented. I will try to reverse-engineer its binary format.

# Definitions:
1. Byte, 8 bits
2. WORD, 16 bits, little-endian
3. DWORD, 32 bits, little-endian

# File Structure
1. Header
1. An Unknown String Section
1. The `Application.Commands` Section
1. The String Section
1. The Unnamed Section
1. The Pointer Section

# The Header
1. 0x0000: Starts with `00 12 00 00 00 00 00 01 00 53 43 42 69 6E` (14 bytes, last 5 bytes in ASCII is `SCBIN`), all binaries seem to share this same header.
1. 0x000e: DWORD, size of the binary file


# An Unknown String Section
Starting at 0x12, this seems to be a string section, with unknown purpose.
1. BYTE, 0x02, stands for the type?
1. DWORD, length of this section, in bytes.
1. BYTE, 0x01, unknown
1. BYTE, the number of strings. \
Each string has the following format: 1 byte of fixed 0x01, 1 WORD of the length of the string and the string itself in ASCII, no '\0' termination. \
These strings can always be found: `Small   MajorItems   StandardItems`

# The `Application.Commands` Section
Almost identical to the XAML `Application.Commands` tags. The length of the section header is 5 bytes. (Fixed)
1. BYTE, 0x0F, stands for the type?
1. DWORD, number of __tags__ within this section

## Tag Format
Each tag consists of multiple __property__ -value pairs. The type is always 1 byte, however, the length of the data payload can vary. Up to now, the length can be 4 or 6, depending on the type byte.
1. DWORD: ID,  __the higher WORD of this DWORD should always be 0, same rule applies to other DWORD IDs.__
1. Byte: Number of property-value pairs
1. Byte: Tag type 1
1. Variable-Length: Tag value 1
1. Byte: Tag type 2
1. Variable-Length: Tag value 2
1. ....

## Property Types:
```
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
```
The first WORD of the payload points to a string or bitmap in the resource file. The second WORD is always 0 (As far as we know). The third WORD (if exists) is the DPI (0x03 to 0x06 only).
The following XAML Attributes never appear in property value:
1. Symbol: Sets the macro name of the resource in the generated .h file
1. Id: Set the id of the resource manually, UICC.exe will generate one if not supplied.
1. Content and Source: Their content are included in the resource file only.

# The String Section
1. BYTE, 0x10, stands for the type?
1. DWORD: length of this String Section in bytes, including this DWORD and previous byte
1. DWORD: number of strings within this String Section

## Each string:
1. WORD, ID
1. WORD, 0x0000
1. WORD, little-endian, object type
1. WORD, 0x1000, little-endian, fixed. __However, there is one exception (up to now, known): for type 0x0001, this WORD is 0x0000.__
1. WORD, length of this string
1. __WORDs__, WideChar representation of the string.

## Object Type
Little-endian:
1. 0x0600 Toggle Button
1. 0x0700 Group
1. 0x0F00 Button
1. 0x1300 "File" Menu
1. 0x1500 Gallery
1. 0x1800 MenuGroup
1. 0x1A00 Tab
1. 0x2500 Quick Access Bar (Qat)

# The Tree Section
## Header
1. BYTE, 0x0D
1. WORD, 0x0003, little-endian, fixed
1. DWORD, the absolute address of the pointer section
1. The root node
1. DWORD, length of the supplementary block section
1. supplementary block 1
1. supplementary block 2
1. supplementary block n

## Supplementary Block format
1. WORD, length of this supplementary block
1. The root collection of this supplementary block, type=0x01 (needs double check).

## Tag types
Up to now, 4 types of nodes have been found. They are distinguished by their first byte.
### Node (0x16)
The root node, and the other nodes follows this format. 
The node can have other nodes, collections, properties and pointer data as child tag.
1. BYTE, tag type, 0x16
1. WORD, object type
1. WORD, 0x1000, little-endian, fixed
1. WORD, length of this node and its children, from the 0x16 byte to the end.
1. BYTE, the sum of numbers of child tags
1. child tags (binary data)
### Collection (0x18)
A collection holds a list of child nodes. It can only have nodes as its child tag.
1. BYTE, tag type, 0x18
1. BYTE, fixed, 0x01
1. BYTE, purpose unknown, seems to be some kind of type indicator
1. WORD, number of child nodes
1. child nodes
### Properties (0x01)
Properties seem to start with 0x01 and followed by 3 bytes which represents its type, and then the data payload. 
The length of data is variable, depending on the property type. The known properties are:
Up to now, there are only 2 known property types.
1. BYTE, tag type, 0x01
1. BYTE, type b1
1. BYTE, type b2
1. BYTE, type b3
1. the payload, variable length

if b1 is 0x01, b3 seems to indicate the length of the payload: 0x04->1 byte, 0x03->2bytes, 0x02->4bytes.

```
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
```
### Pointer Data (0x3E)
They point to addresses within the resource file.
1. BYTE, tag type, 0x3E
1. DWORD, the target address

# The Pointer Section
The first DWORD of this section is the length of this section, including the first DWORD. This section seems to store an array of pointers, each pointer points to the address of the first occurrence of its referring object. The purpose of this section is still not clear.

# References
1. https://www.codeproject.com/Articles/119319/Windows-Ribbon-Framework-in-Win32-C-Application