/***********************************************************************************************************************
MMBasic

Struct.c

User defined structures: TYPE ... END TYPE, structure variables and their members.  The PicoMite's STRUCTENABLED
code, ported to the CMM2 (where structures are always present).

Copyright 2011 - 2026 Geoff Graham.  All Rights Reserved.
Copyright 2016 - 2026 Peter Mather.  All Rights Reserved.

This file and modified versions of this file are supplied to specific individuals or organisations under the following
provisions:

- This file, or any files that comprise the MMBasic source (modified or not), may not be distributed or copied to any other
  person or organisation without written permission.

- Object files (.o and .hex files) generated using this file (modified or not) may not be distributed or copied to any other
  person or organisation without written permission.

- This file is provided in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

************************************************************************************************************************/

#include <string.h>
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"

// The TYPE table: a GetMemory block for each type, numbered in the order the program defines them (a nested type
// must be defined before the type that uses it).  PrepareProgram makes it from the program once for a heap, as it
// makes the name index, so a structure variable's type number (its record's size) holds from RUN to the prompt.
struct s_structdef *g_structtbl[MAX_STRUCT_TYPES];
int g_structcnt = 0;                                                // the number of types defined
int g_StructArg = -1;                                               // the type of a structure being declared (AS type), or -1
int g_StructMemberType = 0;                                         // the type of the member the last findvar() reached, 0 for none
int g_StructMemberOffset = 0;                                       // its offset in the structure
int g_StructMemberSize = 0;                                         // its size: a string's length, a nested structure's type
int g_ExprStructType = -1;                                          // the type of the whole structure an expression gave, or -1
static int StructBuilt;                                             // the table has been made for this heap

// InitHeap: the heap has gone, and the types' memory with it: forget them (there is nothing to free)
void StructTableForget(void) {
    for(int i = 0; i < MAX_STRUCT_TYPES; i++) g_structtbl[i] = NULL;
    g_structcnt = 0;
    g_StructArg = -1;
    StructBuilt = 0;
}

// PrepareProgram: true when the TYPE table is to be made from the program now (once for a heap)
int StructTableStart(void) {
    if(StructBuilt) return 0;
    StructBuilt = 1;
    return 1;
}

// the alignment a structure needs (8 when it holds a number), so the elements of an array of them stay aligned
static int GetStructAlignment(struct s_structdef *sd) {
    int i, align, max_align = 1;
    for(i = 0; i < sd->num_members; i++) {
        struct s_structmember *sm = &sd->members[i];
        align = 1;
        if(sm->type == T_INT || sm->type == T_NBR)
            align = sizeof(long long int);
        else if(sm->type == T_STRUCT)
            align = (sm->size < g_structcnt) ? GetStructAlignment(g_structtbl[sm->size]) : sizeof(long long int);
        if(align > max_align) max_align = align;
    }
    return max_align;
}

// a TYPE block that is wrong: an error at RUN (ErrAbort, on its line), else the block is passed over
static void TypeError(const char *msg, char *line, int ErrAbort) {
    if(ErrAbort) {
        CurrentLinePtr = line;
        error((char *)msg);
    }
}

// PrepareProgramExt: the TYPE command at p begins a block; add its type to the table.  Returns where the scan of the
// program carries on: the end of the END TYPE, or where a block in error was left (when ErrAbort is false).
char *StructDefine(char *p, int ErrAbort) {
    char name[MAXVARLEN + 1], *line = CurrentLinePtr;
    int namelen = 0, j;
    struct s_structdef *sd;
    const char *err = NULL;
    CommandToken tkn, cmdREM = GetCommandValue("Rem");

    p += sizeof(CommandToken);                                      // the type's name
    skipspace(p);
    if(!isnamestart(*p)) {
        TypeError("Invalid TYPE name", line, ErrAbort);
        return p;
    }
    while(isnamechar(*p) && *p != '.') {
        if(namelen == MAXVARLEN) {
            TypeError("TYPE name too long", line, ErrAbort);
            return p;
        }
        name[namelen++] = toupper(*p++);
    }
    name[namelen] = 0;
    for(j = 0; j < g_structcnt; j++)
        if(strcmp(name, g_structtbl[j]->name) == 0) break;
    if(j < g_structcnt) {
        TypeError("TYPE already defined", line, ErrAbort);           // (when not ErrAbort the first one stands)
        return p;
    }
    if(g_structcnt >= MAX_STRUCT_TYPES) {
        TypeError("Too many structure types", line, ErrAbort);
        return p;
    }
    sd = GetMemory(sizeof(struct s_structdef));                     // (zeroed)
    strcpy(sd->name, name);
    g_structtbl[g_structcnt] = sd;                                  // (counted at its END TYPE: ParseStructMember sees the types before it)

    // the members, a line each until END TYPE: they are not commands, so the lines are walked here
    while(*p) p++;                                                  // the end of the TYPE statement
    p++;
    while(1) {
        if(*p == 0) {                                               // the end of the program
            CurrentLinePtr = line;
            err = "No matching END TYPE";
            break;
        }
        if(*p == T_NEWLINE) {
            CurrentLinePtr = p;
            p += T_NEWLINE_HDR;
        }
        if(*p == T_LINENBR) p += 3;
        skipspace(p);
        if(*p == T_LABEL) {
            p += p[1] + 2;
            skipspace(p);
        }
        tkn = commandtbl_at(p);
        if(*p == 0 || *p == '\'' || tkn == cmdREM) {                // a blank line or a comment
            while(*p) p++;
            p++;
            continue;
        }
        if(tkn == cmdEND_TYPE) {
            if(sd->num_members == 0) {
                err = "TYPE has no members";
                break;
            }
            j = GetStructAlignment(sd);                             // pad the size so the elements of an array stay aligned
            sd->total_size = (sd->total_size + j - 1) / j * j;
            g_structcnt++;
            while(*p) p++;
            return p;
        }
        if(tkn != CMD_NOTOKEN) {                                    // a member's name must not be a command
            err = (tkn == cmdTYPE) ? "Nested TYPE not allowed" : "Invalid statement inside TYPE block";
            break;
        }
        if((err = ParseStructMember(p, sd)) != NULL) break;
        while(*p) p++;
        p++;
    }
    TypeError(err, CurrentLinePtr, ErrAbort);
    g_structtbl[g_structcnt] = NULL;                                // (not ErrAbort: the block is passed over)
    FreeMemory(sd);
    return p;
}

// one member of a TYPE block:  name[(dim1[, dim2 ...])] AS INTEGER | FLOAT | STRING [LENGTH n] | type
// NULL when it is added to sd, else what is wrong with it
const char *ParseStructMember(char *p, struct s_structdef *sd) {
    char name[MAXVARLEN + 1], *tp;
    int namelen = 0, type, size, offset, ndims = 0, elements = 1, i, dim;
    int dims[MAXDIM] = {0};
    struct s_structmember *sm;

    if(sd->num_members >= MAX_STRUCT_MEMBERS) return "Too many members in TYPE";
    skipspace(p);
    if(!isnamestart(*p)) return "Invalid member definition in TYPE";
    while(isnamechar(*p) && *p != '.') {
        if(namelen == MAXVARLEN) return "Invalid member definition in TYPE";
        name[namelen++] = toupper(*p++);
    }
    name[namelen] = 0;
    for(i = 0; i < sd->num_members; i++)
        if(strcmp(name, sd->members[i].name) == 0) return "Member already defined in TYPE";
    skipspace(p);

    // its dimensions: numbers only, as nothing can be evaluated while the program is prepared.  The element
    // count uses OPTION BASE as it is then, 0, so a member array has room for either base.
    if(*p == '(') {
        p++;
        skipspace(p);
        while(*p && *p != ')') {
            if(ndims == MAXDIM) return "Dimensions";
            for(dim = 0, i = 0; *p >= '0' && *p <= '9'; i++) {
                dim = dim * 10 + (*p++ - '0');
                if(dim > 0xFFFF) return "Dimensions";
            }
            if(i == 0 || dim < OptionBase) return "Dimensions";
            dims[ndims++] = DimEncode(dim);
            elements *= (dim + 1 - OptionBase);
            skipspace(p);
            if(*p == ',') {
                p++;
                skipspace(p);
            } else if(*p != ')') return "Dimensions";
        }
        if(*p++ != ')' || ndims == 0) return "Dimensions";
        skipspace(p);
    }

    if(*p == tokenAS)
        p++;
    else if((tp = checkstring(p, "AS")) != NULL)
        p = tp;
    else
        return "Invalid member definition in TYPE";
    skipspace(p);

    if((tp = checkstring(p, "INTEGER")) != NULL || (tp = checkstring(p, "INT")) != NULL) {
        type = T_INT;
        size = sizeof(long long int);
        p = tp;
    } else if((tp = checkstring(p, "FLOAT")) != NULL) {
        type = T_NBR;
        size = sizeof(MMFLOAT);
        p = tp;
    } else if((tp = checkstring(p, "STRING")) != NULL) {
        type = T_STR;
        size = MAXSTRLEN;
        p = tp;
        if((tp = checkstring(p, "LENGTH")) != NULL) {               // STRING LENGTH n, as DIM has it
            p = tp;
            skipspace(p);
            for(size = 0; *p >= '0' && *p <= '9'; p++)
                if(size <= MAXSTRLEN) size = size * 10 + (*p - '0');
            if(size < 1) size = 1;
            if(size > MAXSTRLEN) size = MAXSTRLEN;
        }
    } else {                                                        // a type defined before this one
        char tname[MAXVARLEN + 1];
        int tlen = 0;
        while(isnamechar(*p) && *p != '.' && tlen < MAXVARLEN) tname[tlen++] = toupper(*p++);
        tname[tlen] = 0;
        for(i = 0; i < g_structcnt; i++)
            if(strcmp(tname, g_structtbl[i]->name) == 0) break;
        if(tlen == 0 || i == g_structcnt) return "Unknown type in TYPE definition";
        type = T_STRUCT;
        size = i;                                                   // a nested structure's size is its type number
    }
    skipspace(p);
    if(*p != 0 && *p != '\'') return "Invalid member definition in TYPE";

    // its offset: a number, and a nested structure, on an 8 byte boundary; a string where it falls
    offset = sd->total_size;
    if(type != T_STR) offset = (offset + 7) & ~7;
    sm = &sd->members[sd->num_members++];
    strcpy(sm->name, name);
    sm->type = type;
    sm->size = size;
    sm->offset = offset;
    for(i = 0; i < MAXDIM; i++) RAW_DIM(*sm, i) = dims[i];
    if(type == T_STR)
        sd->total_size = offset + (size + 1) * elements;            // a length byte and the characters, for each element
    else if(type == T_STRUCT)
        sd->total_size = offset + g_structtbl[size]->total_size * elements;
    else
        sd->total_size = offset + size * elements;
    return NULL;
}

// the number of the type named at name (it ends at a character that cannot be in a name, or at a '.'), or -1
int FindStructType(char *name) {
    char uname[MAXVARLEN + 1];
    int i, namelen = 0;
    while(isnamechar(*name) && *name != '.' && namelen < MAXVARLEN) uname[namelen++] = toupper(*name++);
    uname[namelen] = 0;
    for(i = 0; i < g_structcnt; i++)
        if(strcmp(uname, g_structtbl[i]->name) == 0) return i;
    return -1;
}

// the member named at membername (it ends at a '.', a '(' or a character that cannot be in a name) in type
// struct_idx: its index, with its type, offset, size and dimensions where the pointers are not NULL; or -1
int FindStructMember(int struct_idx, char *membername, int *member_type, int *member_offset, int *member_size, int *member_dims) {
    char uname[MAXVARLEN + 1];
    int i, j, namelen = 0;
    struct s_structdef *sd;
    if(struct_idx < 0 || struct_idx >= g_structcnt) return -1;
    sd = g_structtbl[struct_idx];
    while(isnamechar(*membername) && *membername != '.' && namelen < MAXVARLEN) uname[namelen++] = toupper(*membername++);
    uname[namelen] = 0;
    for(i = 0; i < sd->num_members; i++) {
        if(strcmp(uname, sd->members[i].name) == 0) {
            if(member_type) *member_type = sd->members[i].type;
            if(member_offset) *member_offset = sd->members[i].offset;
            if(member_size) *member_size = sd->members[i].size;
            if(member_dims)
                for(j = 0; j < MAXDIM; j++) member_dims[j] = RAW_DIM(sd->members[i], j);
            return i;
        }
    }
    return -1;
}

// TYPE name: at run time the block is passed over (PrepareProgram made the type from it)
void cmd_type(void) {
    char *p = nextstmt;
    CommandToken tkn;
    while(1) {
        p = GetNextCommand(p, NULL, "No matching END TYPE");
        tkn = commandtbl_at(p);
        if(tkn == cmdTYPE) error("Nested TYPE not allowed");
        if(tkn == cmdEND_TYPE) {
            skipelement(p);
            nextstmt = p;
            return;
        }
    }
}

// END TYPE: only reached without its TYPE
void cmd_endtype(void) {
    error("END TYPE without TYPE");
}

// LIST TYPE [name]: the types with their members, offsets and sizes
void ListTypes(char *p) {
    int i, j, d, found = -1;
    skipspace(p);
    if(*p && *p != '\'') {
        if((found = FindStructType(p)) < 0) error("TYPE not found");
    }
    if(g_structcnt == 0) {
        MMPrintString("No structure types defined\r\n");
        return;
    }
    for(i = 0; i < g_structcnt; i++) {
        struct s_structdef *sd = g_structtbl[i];
        if(found >= 0 && i != found) continue;
        MMPrintString("TYPE ");
        MMPrintString(sd->name);
        MMPrintString("\r\n");
        for(j = 0; j < sd->num_members; j++) {
            struct s_structmember *sm = &sd->members[j];
            MMPrintString("    ");
            MMPrintString(sm->name);
            if(DimIsAllocated(RAW_DIM(*sm, 0))) {
                for(d = 0; d < MAXDIM && !DimIsEnd(RAW_DIM(*sm, d)); d++) {
                    MMPrintString(d ? "," : "(");
                    PInt(DimUpper(RAW_DIM(*sm, d)));
                }
                MMPrintString(")");
            }
            MMPrintString(" AS ");
            if(sm->type == T_INT)
                MMPrintString("INTEGER");
            else if(sm->type == T_NBR)
                MMPrintString("FLOAT");
            else if(sm->type == T_STR) {
                MMPrintString("STRING LENGTH ");
                PInt(sm->size);
            } else
                MMPrintString(g_structtbl[sm->size]->name);
            MMPrintString("  ' offset=");
            PInt(sm->offset);
            MMPrintString("\r\n");
        }
        MMPrintString("END TYPE  ' size=");
        PInt(sd->total_size);
        MMPrintString(" bytes\r\n\r\n");
    }
}

// p is at a bracket: the character after its closing bracket (a bracket in a string is not counted)
static char *SkipBrackets(char *p) {
    int depth = 0;
    do {
        if(*p == '"') {
            while(*++p && *p != '"');
            if(*p == 0) break;
        } else if(*p == '(' || (tokentype(*p) & T_FUN))
            depth++;
        else if(*p == ')')
            depth--;
        p++;
    } while(depth && *p);
    if(depth) error("Expected closing bracket");
    return p;
}

// the element (i [, j ...]) at *pp of a member array with dimensions dims: its number in the array, bounds
// checked.  *pp is left after the closing bracket.
static int StructElement(char **pp, int *dims) {
    int i, n, mult, ndims, index[MAXDIM];
    char *p = *pp;
    {
        getargs(&p, MAXDIM * 2, "(,");
        if((argc & 0x01) == 0) error("Dimensions");
        ndims = argc / 2 + 1;
        for(i = 0; i < argc; i += 2) {
            MMFLOAT f;
            long long int in;
            char *s;
            int targ = T_NOTYPE;
            evaluate(argv[i], &f, &in, &s, &targ, false);
            if(targ == T_STR) error("Dimensions");
            if(targ == T_NBR) in = FloatToInt32(f);
            else if(in != (int)in) error("Index out of bounds");    // too big for any array: must not wrap to a small index
            index[i / 2] = in;
        }
    }
    for(n = 0; n < MAXDIM && !DimIsEnd(dims[n]); n++);
    if(ndims != n) error("Wrong number of dimensions");
    for(i = 0; i < ndims; i++)
        if(index[i] < OptionBase || index[i] > DimUpper(dims[i])) error("Index out of bounds");
    n = index[0] - OptionBase;
    for(mult = 1, i = 1; i < ndims; i++) {
        mult *= DimElements(dims[i - 1]);
        n += (index[i] - OptionBase) * mult;
    }
    *pp = SkipBrackets(*pp);
    return n;
}

// The member of the structure of type struct_idx at struct_ptr named by member_path ("X", or "INNER.X" when the
// name held more of the path), continued in the program at *pp: an index for an array member, and more members
// after it (pts(1).corner(2).x).  Returns a pointer to its data, with *member_type its type (a nested structure is
// T_STRUCT), g_StructMemberOffset its offset in the structure and g_StructMemberSize its size (a string's length, a
// nested structure's type).  *pp is left after the path.
void *ResolveStructMember(char *struct_ptr, int struct_idx, char *member_path, char **pp, int *member_type) {
    char name[MAXVARLEN + 1], cont[MAXVARLEN + 1], *p = *pp, *member = member_path, *next, *q;
    int depth = 0, offset = 0, element = 0, len, more, m_type, m_offset, m_size, m_dims[MAXDIM];

    while(1) {
        if(++depth > MAX_STRUCT_NEST_DEPTH) error("Structure nesting too deep (max %)", MAX_STRUCT_NEST_DEPTH);
        for(len = 0; member[len] && member[len] != '.' && member[len] != '(' && len < MAXVARLEN; len++) name[len] = member[len];
        name[len] = 0;
        next = (member[len] == '.') ? member + len + 1 : NULL;     // more of the path in the name
        if(FindStructMember(struct_idx, name, &m_type, &m_offset, &m_size, m_dims) < 0) error("Unknown structure member");
        offset += m_offset;

        // is there more: in the name, or in the program after this member (and its index)
        skipspace(p);
        more = (next != NULL || *p == '.');
        if(!more && m_type == T_STRUCT && DimIsAllocated(m_dims[0]) && *p == '(') {
            q = SkipBrackets(p);
            skipspace(q);
            more = (*q == '.');
        }
        if(more) {
            if(m_type != T_STRUCT) error("Not a nested structure");
            if(DimIsAllocated(m_dims[0])) {                         // an array of structures: the element
                if(*p != '(') error("Array of nested structures requires index");
                offset += StructElement(&p, m_dims) * g_structtbl[m_size]->total_size;
            }
            struct_idx = m_size;
            if(next != NULL)
                member = next;
            else {                                                  // the rest of the path is in the program
                skipspace(p);
                for(p++, len = 0; isnamechar(*p) && len < MAXVARLEN; len++) cont[len] = *p++;
                cont[len] = 0;
                if(*p == '$' || *p == '%' || *p == '!') p++;
                member = cont;                                      // (a local: an index evaluated below may resolve another path)
            }
            continue;
        }

        // the last member: an element of it if it is an array
        if(*p == '(') {
            if(!DimIsAllocated(m_dims[0])) error("Not an array member");
            if(m_type == T_STR) len = m_size + 1;
            else if(m_type == T_STRUCT) len = g_structtbl[m_size]->total_size;
            else len = m_size;
            element = StructElement(&p, m_dims) * len;
        } else if(DimIsAllocated(m_dims[0]) && m_type != T_STRUCT)
            error("Array member requires index");
        g_StructMemberOffset = offset;
        g_StructMemberSize = m_size;
        *member_type = m_type;
        *pp = p;
        return struct_ptr + offset + element;
    }
}

// findvar: the member that *pp ('.' and the path) names in the structure of type struct_idx at struct_ptr
void *StructMemberAt(char *struct_ptr, int struct_idx, char **pp) {
    char name[MAXVARLEN + 1], *p = *pp + 1;
    int n = 0, type;
    void *r;
    while(isnamechar(*p) && n < MAXVARLEN) name[n++] = *p++;
    name[n] = 0;
    if(*p == '$' || *p == '%' || *p == '!') p++;
    r = ResolveStructMember(struct_ptr, struct_idx, name, &p, &type);
    g_StructMemberType = type;
    *pp = p;
    return r;
}
