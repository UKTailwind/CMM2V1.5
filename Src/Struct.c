/***************************************************************************

CMM2 MMBasic
Struct.c

TYPE ... END TYPE, structure variables and their members, the STRUCT command and the STRUCT() function

Copyright 2011-2026 Geoff Graham, Peter Mather and Gerry Allardice.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holders nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

4. The name MMBasic be used when referring to the interpreter in any
   documentation and promotional material and the original copyright message
  be displayed  on the console at startup (additional copyright messages may
   be added).

5. All advertising materials mentioning features or use of this software must
   display the following acknowledgement: This product includes software
   developed by Geoff Graham and Peter Mather.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#include <string.h>
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "re.h"

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
// The name at *pp, a symbol (Symbols.h) or text, in capitals as far as a '.' and at most MAXVARLEN characters,
// into name: its length, 0 if there is none, or -1 if it is longer.  *pp is left after it (text: at the '.').
static int StructNameAt(char **pp, char *name) {
    char *p = *pp;
    const char *s;
    int len, n;
    if(issymbol(*p)) {
        s = SymSpelling(p, &len);
        p += symbolsize(*p);
    } else {
        s = p;
        len = 0;
        if(isnamestart(*p))
            while(isnamechar(p[len]) && p[len] != '.') len++;
        p += len;
    }
    for(n = 0; n < len && s[n] != '.'; n++) {
        if(n == MAXVARLEN) { *pp = p; return -1; }
        name[n] = toupper(s[n]);
    }
    name[n] = 0;
    *pp = p;
    return n;
}

char *StructDefine(char *p, int ErrAbort) {
    char name[MAXVARLEN + 1], *line = CurrentLinePtr;
    int namelen = 0, j;
    struct s_structdef *sd;
    const char *err = NULL;
    CommandToken tkn, cmdREM = GetCommandValue("Rem");

    p += sizeof(CommandToken);                                      // the type's name
    skipspace(p);
    if(!isnamestartsym(*p)) {
        TypeError("Invalid TYPE name", line, ErrAbort);
        return p;
    }
    if((namelen = StructNameAt(&p, name)) < 0) {
        TypeError("TYPE name too long", line, ErrAbort);
        return p;
    }
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
    if(!isnamestartsym(*p)) return "Invalid member definition in TYPE";
    if((namelen = StructNameAt(&p, name)) < 0) return "Invalid member definition in TYPE";
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
        int tlen = StructNameAt(&p, tname);
        if(tlen < 0) tlen = 0;                                      // (too long: not a type)
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
    int i;
    if(StructNameAt(&name, uname) <= 0) return -1;                  // (a symbol or text)
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
                p = CopyName(p + 1, cont, &len);                    // (a symbol or text)
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
    p = CopyName(p, name, &n);                                      // (a symbol or text)
    if(*p == '$' || *p == '%' || *p == '!') p++;
    r = ResolveStructMember(struct_ptr, struct_idx, name, &p, &type);
    g_StructMemberType = type;
    *pp = p;
    return r;
}

// DefinedSubFun: a parameter AS a TYPE's name (vindex, which findvar made with data of its own) takes the caller's
// structure, by reference.  argtype and argstruct are the argument's type and structure type, which for a member
// (r.topLeft) are the member's own: the PicoMite's checked the base variable's, the outer type.  A whole array of
// structures given for it (arr()) makes it that array, as on the PicoMite.
void StructParam(int vindex, int argvindex, char *data, int argtype, int argstruct, char *argname) {
    char *p;
    if(!(argtype & T_PTR) || !(argtype & T_STRUCT)) error("Expected a structure variable: $", argname);
    if(VREC(vindex)->size != argstruct) error("Structure type mismatch: $", argname);
    if(VREC(vindex)->val.s != NULL) FreeMemorySafe((void **)&VREC(vindex)->val.s);
    VREC(vindex)->val.s = data;
    VREC(vindex)->type |= T_PTR;                                    // (ClearVars does not free it)
    if(DimIsAllocated(VREC(argvindex)->dimtbl[0]) && (p = strchr(argname, '(')) != NULL) {
        p++;
        skipspace(p);
        if(*p == ')')
            for(int j = 0; j < MAXDIM; j++) VREC(vindex)->dimtbl[j] = VREC(argvindex)->dimtbl[j];
    }
}

// DefinedSubFun: a FUNCTION's structure, copied to temporary memory at the caller's level (its own goes with its
// locals), with g_ExprStructType its type for the assignment that takes it
char *StructReturn(char *data, int type) {
    int n = g_structtbl[type]->total_size;
    char *t;
    LocalIndex--;
    t = GetTempMemory(n);
    LocalIndex++;
    memcpy(t, data, n);
    g_ExprStructType = type;
    return t;
}

// DIM ... AS type = (v1, v2 ...): a value for each member of each of the n structures at data, in order, a member
// array's elements in turn, each string no longer than its member's LENGTH.  p is at the '('; returns after the ')'.
char *StructInit(char *p, int sidx, char *data, int n) {
    struct s_structdef *sd = g_structtbl[sidx];
    struct s_structmember *sm;
    int e, m, k, count, esize;
    char *mp;
    if(*p != '(') error("Expected '(' for structure initialisation");
    p++;
    for(e = 0; e < n; e++, data += sd->total_size) {
        for(m = 0; m < sd->num_members; m++) {
            sm = &sd->members[m];
            if(sm->type == T_STRUCT) error("A nested structure cannot be initialised");
            esize = (sm->type == T_STR) ? sm->size + 1 : 8;
            for(count = 1, k = 0; k < MAXDIM && !DimIsEnd(sm->dimtbl[k]); k++) count *= DimElements(sm->dimtbl[k]);
            for(mp = data + sm->offset; count--; mp += esize) {
                skipspace(p);
                if(*p == ')' || *p == 0) error("Not enough initialisation values");
                p = SetValue(p, sm->type, mp, sm->size);
                skipspace(p);
                if(*p == ',')
                    p++;
                else if(*p != ')')
                    error("Expected ',' or ')' in structure initialisation");
            }
        }
    }
    skipspace(p);
    if(*p != ')') error("Too many initialisation values");
    return p + 1;
}


// STRUCT LOAD: count bytes from the open disk file fnbr (FilePutStr writes); the number read
static unsigned int StructFileRead(int fnbr, void *buff, int count) {
    unsigned int nbr = 0;
    InitSDCard();
    FSerror = f_read(FileTable[fnbr].fptr, buff, count, &nbr);
    ErrorCheck(fnbr);
    return nbr;
}

// STRUCT command - operations on structure variables
// Syntax:
//   STRUCT COPY source TO destination
//   STRUCT COPY source() TO destination()  - copy entire array
//   (future: STRUCT PRINT var, STRUCT CLEAR var, etc.)
void cmd_struct(void)
{
	char *p;

	if ((p = checkstring(cmdline, (char *)"COPY")) != NULL)
	{
		// STRUCT COPY source TO destination
		// STRUCT COPY source() TO destination()  - copy entire array
		char *src_ptr, *dst_ptr;
		int src_idx, dst_idx, src_struct_type, dst_struct_type;
		char *tp;
		int src_is_array = 0, dst_is_array = 0;
		int src_num_elements = 1, dst_num_elements = 1;

		skipspace(p);

		// Get source variable - V_EMPTY_OK allows empty () for arrays
		src_ptr = findvar(p, V_FIND | V_NOFIND_ERR | V_EMPTY_OK);
		src_idx = VarIndex;

		// Check source is a struct (not a member access)
		if (!(VREC(src_idx)->type & T_STRUCT))
			error("Source must be a structure variable");

		// For struct arrays, the pointer returned is to the specific element
		// but we need to verify this is a whole struct, not a member
		if (g_StructMemberType != 0)
			error("Cannot copy structure member, use whole structure");

		src_struct_type = (int)VREC(src_idx)->size; // struct type index stored in size field

		// Check if source is an array with empty parentheses (whole array copy)
		// V_EMPTY_OK returns base pointer when () is empty
		if (DimIsAllocated(RAW_DIM((*VREC(src_idx)), 0)))
		{
			// It's an array - check if empty parentheses were used
			char *paren = (char *)strchr((char *)p, '(');
			if (paren)
			{
				paren++;
				skipspace(paren);
				if (*paren == ')')
				{
					// Empty parentheses - whole array copy
					src_is_array = 1;
					for (int d = 0; d < MAXDIM && !DimIsEnd(RAW_DIM((*VREC(src_idx)), d)); d++)
					{
						src_num_elements *= DimElements(RAW_DIM((*VREC(src_idx)), d));
					}
				}
			}
		}

		// Skip past the source variable to find TO
		tp = skipvar(p, false);
		skipspace(tp);

		// Check for TO keyword (tokenized)
		if (*tp != tokenTO)
			error("Expected TO");
		tp++; // skip TO token
		skipspace(tp);

		// Get destination variable
		dst_ptr = findvar(tp, V_FIND | V_NOFIND_ERR | V_EMPTY_OK);
		dst_idx = VarIndex;

		// Check destination is a struct
		if (!(VREC(dst_idx)->type & T_STRUCT))
			error("Destination must be a structure variable");

		if (g_StructMemberType != 0)
			error("Cannot copy to structure member, use whole structure");

		dst_struct_type = (int)VREC(dst_idx)->size;

		// Check if destination is an array with empty parentheses
		if (DimIsAllocated(RAW_DIM((*VREC(dst_idx)), 0)))
		{
			char *paren = (char *)strchr((char *)tp, '(');
			if (paren)
			{
				paren++;
				skipspace(paren);
				if (*paren == ')')
				{
					dst_is_array = 1;
					for (int d = 0; d < MAXDIM && !DimIsEnd(RAW_DIM((*VREC(dst_idx)), d)); d++)
					{
						dst_num_elements *= DimElements(RAW_DIM((*VREC(dst_idx)), d));
					}
				}
			}
		}

		// Validate same struct type
		if (src_struct_type != dst_struct_type)
			error("Structure types must match");

		// Validate array copy consistency
		if (src_is_array != dst_is_array)
			error("Both source and destination must be arrays or both must be single structs");

		// For array copy, destination must be at least as large as source
		if (src_is_array && dst_num_elements < src_num_elements)
			error("Destination array too small");

		// Perform the copy
		int struct_size = g_structtbl[src_struct_type]->total_size;
		int copy_size = struct_size * src_num_elements;
		memcpy(dst_ptr, src_ptr, copy_size);
	}
	else if ((p = checkstring(cmdline, (char *)"SORT")) != NULL)
	{
		// STRUCT SORT array().membername [, flags]
		// flags: bit0=reverse, bit1=case insensitive (strings), bit2=empty strings at end (strings)
		int arr_idx, struct_type;
		char *tp;
		int flags = 0;
		int member_type, member_offset, member_size;
		int num_elements;
		int struct_size;

		skipspace(p);

		// Get array variable with member access: array().membername
		// findvar will resolve the member access and set g_StructMemberType/Offset/Size
		findvar(p, V_FIND | V_NOFIND_ERR | V_EMPTY_OK);
		arr_idx = VarIndex;

		// Check it's a struct array with member access
		if (!(VREC(arr_idx)->type & T_STRUCT))
			error("Expected a structure array");
		if (DimIsScalar(RAW_DIM((*VREC(arr_idx)), 0)))
			error("Expected a structure array");
		if (g_StructMemberType == 0)
			error("Expected structarray().membername syntax");

		// Get member info from globals set by findvar
		member_type = g_StructMemberType;
		member_offset = g_StructMemberOffset;
		member_size = g_StructMemberSize;
		(void)member_size; // Used for validation, suppress unused warning

		// Member cannot be a nested struct
		if (member_type == T_STRUCT)
			error("Cannot sort by nested structure member");

		struct_type = (int)VREC(arr_idx)->size;
		struct_size = g_structtbl[struct_type]->total_size;

		// Calculate number of elements
		num_elements = 1;
		for (int d = 0; d < MAXDIM && !DimIsEnd(RAW_DIM((*VREC(arr_idx)), d)); d++)
		{
			num_elements *= DimElements(RAW_DIM((*VREC(arr_idx)), d));
		}

		// Skip past array().membername to find optional flags
		tp = skipvar(p, false);
		skipspace(tp);

		// Check for optional flags parameter
		if (*tp == ',')
		{
			tp++;
			skipspace(tp);
			flags = (int)getint(tp, 0, 7);
		}

		// Get pointer to array data
		char *base_ptr = VREC(arr_idx)->val.s;

		// Allocate temporary buffer for swap
		char *temp = GetTempMemory(struct_size);

		// Shell sort implementation (efficient for medium arrays, in-place)
		int gap, i, j;
		int reverse = flags & 1;
		int case_insensitive = flags & 2;
		int empty_at_end = flags & 4;

		for (gap = num_elements / 2; gap > 0; gap /= 2)
		{
			for (i = gap; i < num_elements; i++)
			{
				memcpy(temp, base_ptr + i * struct_size, struct_size);

				for (j = i; j >= gap; j -= gap)
				{
					char *elem_j_gap = base_ptr + (j - gap) * struct_size;
					char *val_a = elem_j_gap + member_offset;
					char *val_b = temp + member_offset;
					int cmp = 0;

					// Compare based on member type
					if (member_type & T_INT)
					{
						long long int a = *(long long int *)val_a;
						long long int b = *(long long int *)val_b;
						if (a < b)
							cmp = -1;
						else if (a > b)
							cmp = 1;
						else
							cmp = 0;
					}
					else if (member_type & T_NBR)
					{
						MMFLOAT a = *(MMFLOAT *)val_a;
						MMFLOAT b = *(MMFLOAT *)val_b;
						if (a < b)
							cmp = -1;
						else if (a > b)
							cmp = 1;
						else
							cmp = 0;
					}
					else if (member_type & T_STR)
					{
						// MMBasic strings: first byte is length
						int len_a = *val_a;
						int len_b = *val_b;

						// Handle empty strings at end option
						if (empty_at_end)
						{
							if (len_a == 0 && len_b != 0)
							{
								cmp = 1; // Empty string a goes after b
							}
							else if (len_a != 0 && len_b == 0)
							{
								cmp = -1; // Non-empty a goes before empty b
							}
							else if (len_a == 0 && len_b == 0)
							{
								cmp = 0; // Both empty, equal
							}
							else
							{
								// Both non-empty, compare normally
								int minlen = (len_a < len_b) ? len_a : len_b;
								if (case_insensitive)
								{
									for (int k = 1; k <= minlen; k++)
									{
										int ca = toupper(val_a[k]);
										int cb = toupper(val_b[k]);
										if (ca < cb)
										{
											cmp = -1;
											break;
										}
										if (ca > cb)
										{
											cmp = 1;
											break;
										}
									}
								}
								else
								{
									for (int k = 1; k <= minlen; k++)
									{
										if (val_a[k] < val_b[k])
										{
											cmp = -1;
											break;
										}
										if (val_a[k] > val_b[k])
										{
											cmp = 1;
											break;
										}
									}
								}
								if (cmp == 0)
								{
									if (len_a < len_b)
										cmp = -1;
									else if (len_a > len_b)
										cmp = 1;
								}
							}
						}
						else
						{
							// Normal string comparison
							int minlen = (len_a < len_b) ? len_a : len_b;
							if (case_insensitive)
							{
								for (int k = 1; k <= minlen; k++)
								{
									int ca = toupper(val_a[k]);
									int cb = toupper(val_b[k]);
									if (ca < cb)
									{
										cmp = -1;
										break;
									}
									if (ca > cb)
									{
										cmp = 1;
										break;
									}
								}
							}
							else
							{
								for (int k = 1; k <= minlen; k++)
								{
									if (val_a[k] < val_b[k])
									{
										cmp = -1;
										break;
									}
									if (val_a[k] > val_b[k])
									{
										cmp = 1;
										break;
									}
								}
							}
							if (cmp == 0)
							{
								if (len_a < len_b)
									cmp = -1;
								else if (len_a > len_b)
									cmp = 1;
							}
						}
					}

					// Apply reverse flag
					if (reverse)
						cmp = -cmp;

					// If elem[j-gap] > temp, shift it up
					if (cmp > 0)
					{
						memcpy(base_ptr + j * struct_size, elem_j_gap, struct_size);
					}
					else
					{
						break;
					}
				}
				memcpy(base_ptr + j * struct_size, temp, struct_size);
			}
		}
	}
	else if ((p = checkstring(cmdline, (char *)"CLEAR")) != NULL)
	{
		// STRUCT CLEAR var or STRUCT CLEAR array()
		// Resets all members to defaults (0 for numbers, "" for strings)
		int var_idx, struct_type;
		char *var_ptr;
		int struct_size;

		skipspace(p);

		// Get variable - V_EMPTY_OK allows empty () for arrays
		var_ptr = findvar(p, V_FIND | V_NOFIND_ERR | V_EMPTY_OK);
		var_idx = VarIndex;

		// Check it's a struct
		if (!(VREC(var_idx)->type & T_STRUCT))
			error("Expected a structure variable");

		if (g_StructMemberType != 0)
			error("Cannot clear a structure member, use whole structure");

		struct_type = (int)VREC(var_idx)->size;
		struct_size = g_structtbl[struct_type]->total_size;

		// Calculate total size if array
		int num_elements = 1;
		if (DimIsAllocated(RAW_DIM((*VREC(var_idx)), 0)))
		{
			for (int d = 0; d < MAXDIM && !DimIsEnd(RAW_DIM((*VREC(var_idx)), d)); d++)
			{
				num_elements *= DimElements(RAW_DIM((*VREC(var_idx)), d));
			}
		}

		// Zero the memory
		memset(var_ptr, 0, struct_size * num_elements);
	}
	else if ((p = checkstring(cmdline, (char *)"SWAP")) != NULL)
	{
		// STRUCT SWAP var1, var2
		// Swaps two struct variables (must be same type)
		char *src_ptr, *dst_ptr;
		int src_idx, dst_idx;
		int src_struct_type, dst_struct_type;
		char *tp;

		skipspace(p);

		// Get first variable
		src_ptr = findvar(p, V_FIND | V_NOFIND_ERR);
		src_idx = VarIndex;

		if (!(VREC(src_idx)->type & T_STRUCT))
			error("First argument must be a structure variable");

		if (g_StructMemberType != 0)
			error("Cannot swap structure member, use whole structure");

		src_struct_type = (int)VREC(src_idx)->size;

		// Skip past first variable to find comma
		tp = skipvar(p, false);
		skipspace(tp);

		if (*tp != ',')
			error("Expected comma");
		tp++;
		skipspace(tp);

		// Get second variable
		dst_ptr = findvar(tp, V_FIND | V_NOFIND_ERR);
		dst_idx = VarIndex;

		if (!(VREC(dst_idx)->type & T_STRUCT))
			error("Second argument must be a structure variable");

		if (g_StructMemberType != 0)
			error("Cannot swap structure member, use whole structure");

		dst_struct_type = (int)VREC(dst_idx)->size;

		// Validate same struct type
		if (src_struct_type != dst_struct_type)
			error("Structure types must match");

		// Perform the swap using temp memory
		int swap_size = g_structtbl[src_struct_type]->total_size;
		char *temp = GetTempMemory(swap_size);
		memcpy(temp, src_ptr, swap_size);
		memcpy(src_ptr, dst_ptr, swap_size);
		memcpy(dst_ptr, temp, swap_size);
	}
	else if ((p = checkstring(cmdline, (char *)"SAVE")) != NULL)
	{
		// STRUCT SAVE #n, var or STRUCT SAVE #n, array() or STRUCT SAVE #n, array(i)
		// Writes struct data as binary to open file
		int fnbr;
		int var_idx, struct_type, struct_size;
		char *var_ptr;

		skipspace(p);

		// Get file number
		if (*p != '#')
			error("Expected #filenumber");
		p++;
		fnbr = getinteger(p);

		// Validate file number and that it's a disk file
		if (fnbr < 1 || fnbr > MAXOPENFILES)
			error("Invalid file number");
		if (FileTable[fnbr].com == 0)
			error("File not open");
		if (FileTable[fnbr].com <= MAXCOMPORTS)
			error("Not a disk file");

		// Skip past file number to comma, past its brackets (#MAX(1, 0): the PicoMite's #106)
		p = skipexpression(p);
		if (*p != ',')
			error("Expected comma");
		p++;
		skipspace(p);

		// Save pointer to variable name for parenthesis check
		char *varname = p;

		// Get variable - V_EMPTY_OK allows empty () for arrays
		var_ptr = findvar(p, V_FIND | V_NOFIND_ERR | V_EMPTY_OK);
		var_idx = VarIndex;

		if (!(VREC(var_idx)->type & T_STRUCT))
			error("Expected a structure variable");

		if (g_StructMemberType != 0)
			error("Cannot save a structure member, use whole structure");

		struct_type = (int)VREC(var_idx)->size;
		struct_size = g_structtbl[struct_type]->total_size;

		// Determine if it's an array and how to handle it
		int num_elements = 1;
		int is_array = DimIsAllocated(RAW_DIM((*VREC(var_idx)), 0));

		if (is_array)
		{
			// Check for parentheses
			char *paren = (char *)strchr((char *)varname, '(');
			if (!paren)
				error("Array variable requires () or (index)");

			paren++;
			skipspace(paren);
			if (*paren == ')')
			{
				// Empty brackets - save entire array
				for (int d = 0; d < MAXDIM && !DimIsEnd(RAW_DIM((*VREC(var_idx)), d)); d++)
				{
					num_elements *= DimElements(RAW_DIM((*VREC(var_idx)), d));
				}
			}
			else
			{
				// Has index - findvar already resolved to the correct element
				// var_ptr points to the specific element, save just one
				num_elements = 1;
			}
		}

		// Write struct data to file
		FilePutStr(struct_size * num_elements, (char *)var_ptr, fnbr);
	}
	else if ((p = checkstring(cmdline, (char *)"LOAD")) != NULL)
	{
		// STRUCT LOAD #n, var or STRUCT LOAD #n, array() or STRUCT LOAD #n, array(i)
		// Reads struct data as binary from open file
		int fnbr;
		int var_idx, struct_type, struct_size;
		char *var_ptr;
		unsigned int bytes_read;

		skipspace(p);

		// Get file number
		if (*p != '#')
			error("Expected #filenumber");
		p++;
		fnbr = getinteger(p);

		// Validate file number and that it's a disk file
		if (fnbr < 1 || fnbr > MAXOPENFILES)
			error("Invalid file number");
		if (FileTable[fnbr].com == 0)
			error("File not open");
		if (FileTable[fnbr].com <= MAXCOMPORTS)
			error("Not a disk file");

		// Skip past file number to comma, past its brackets (#MAX(1, 0): the PicoMite's #106)
		p = skipexpression(p);
		if (*p != ',')
			error("Expected comma");
		p++;
		skipspace(p);

		// Save pointer to variable name for parenthesis check
		char *varname = p;

		// Get variable - V_EMPTY_OK allows empty () for arrays
		var_ptr = findvar(p, V_FIND | V_NOFIND_ERR | V_EMPTY_OK);
		var_idx = VarIndex;

		if (!(VREC(var_idx)->type & T_STRUCT))
			error("Expected a structure variable");

		if (g_StructMemberType != 0)
			error("Cannot load into a structure member, use whole structure");

		struct_type = (int)VREC(var_idx)->size;
		struct_size = g_structtbl[struct_type]->total_size;

		// Determine if it's an array and how to handle it
		int num_elements = 1;
		int is_array = DimIsAllocated(RAW_DIM((*VREC(var_idx)), 0));

		if (is_array)
		{
			// Check for parentheses
			char *paren = (char *)strchr((char *)varname, '(');
			if (!paren)
				error("Array variable requires () or (index)");

			paren++;
			skipspace(paren);
			if (*paren == ')')
			{
				// Empty brackets - load entire array
				for (int d = 0; d < MAXDIM && !DimIsEnd(RAW_DIM((*VREC(var_idx)), d)); d++)
				{
					num_elements *= DimElements(RAW_DIM((*VREC(var_idx)), d));
				}
			}
			else
			{
				// Has index - findvar already resolved to the correct element
				// var_ptr points to the specific element, load just one
				num_elements = 1;
			}
		}

		// Read struct data from file
		bytes_read = StructFileRead(fnbr, var_ptr, struct_size * num_elements);
		if(bytes_read != struct_size * num_elements) error("Not enough data in the file");   // (a structure left part loaded)
	}
	else if ((p = checkstring(cmdline, (char *)"PRINT")) != NULL)
	{
		// STRUCT PRINT var or STRUCT PRINT array() or STRUCT PRINT array(n)
		// Prints all members of a structure for debugging
		int var_idx, struct_type, struct_size;
		char *var_ptr;
		struct s_structdef *sd;

		skipspace(p);

		// Get variable - V_EMPTY_OK allows empty () for arrays
		var_ptr = findvar(p, V_FIND | V_NOFIND_ERR | V_EMPTY_OK);
		var_idx = VarIndex;

		if (!(VREC(var_idx)->type & T_STRUCT))
			error("Expected a structure variable");

		if (g_StructMemberType != 0)
			error("Cannot print a structure member, use whole structure");

		struct_type = (int)VREC(var_idx)->size;
		struct_size = g_structtbl[struct_type]->total_size;
		sd = g_structtbl[struct_type];

		// Calculate number of elements to print
		int num_elements = 1;
		int is_array = DimIsAllocated(RAW_DIM((*VREC(var_idx)), 0));
		int single_element = 0;

		// Check if this is an indexed array access (e.g., arr(2)) vs whole array (arr())
		// If findvar resolved to a specific element, var_ptr points to that element
		// We detect this by checking if parentheses contain a value
		char *paren = (char *)strchr((char *)p, '(');
		if (paren && is_array)
		{
			paren++;
			skipspace(paren);
			if (*paren != ')')
			{
				// Has an index - print single element
				single_element = 1;
				num_elements = 1;
			}
		}

		if (is_array && !single_element)
		{
			for (int d = 0; d < MAXDIM && !DimIsEnd(RAW_DIM((*VREC(var_idx)), d)); d++)
			{
				num_elements *= DimElements(RAW_DIM((*VREC(var_idx)), d));
			}
		}

		// Print structure type name
		MMPrintString((char *)sd->name);
		if (is_array && !single_element)
		{
			char buf[32];
			sprintf(buf, " array (%d elements):\r\n", num_elements);
			MMPrintString(buf);
		}
		else if (single_element)
		{
			MMPrintString(":\r\n");
		}
		else
		{
			MMPrintString(":\r\n");
		}

		// Print each element
		for (int elem = 0; elem < num_elements; elem++)
		{
			char *elem_ptr = var_ptr + (elem * struct_size);

			if (is_array && !single_element)
			{
				char buf[32];
				sprintf(buf, "[%d]:\r\n", elem + OptionBase);
				MMPrintString(buf);
			}

			// Print each member
			for (int m = 0; m < sd->num_members; m++)
			{
				struct s_structmember *sm = &sd->members[m];
				char *member_ptr = elem_ptr + sm->offset;
				char buf[STRINGSIZE];

				// Calculate array elements for this member
				int member_elements = 1;
				for (int d = 0; d < MAXDIM && !DimIsEnd(RAW_DIM(*sm, d)); d++)
				{
					member_elements *= DimElements(RAW_DIM(*sm, d));
				}

				if (sm->type == T_STRUCT)
				{
					// Nested structure - print recursively with indentation
					struct s_structdef *nested_sd = g_structtbl[sm->size];
					sprintf(buf, "  .%s = %s:\r\n", sm->name, nested_sd->name);
					MMPrintString(buf);

					// Print nested members with extra indent
					for (int nm = 0; nm < nested_sd->num_members; nm++)
					{
						struct s_structmember *nsm = &nested_sd->members[nm];
						char *nested_ptr = member_ptr + nsm->offset;

						sprintf(buf, "    .%s = ", nsm->name);
						MMPrintString(buf);

						if (nsm->type == T_INT)
						{
							long long int val = *(long long int *)nested_ptr;
							IntToStr(buf, val, 10);                         // (newlib nano has no %lld: it printed "ld")
							MMPrintString(buf);
						}
						else if (nsm->type == T_NBR)
						{
							MMFLOAT val = *(MMFLOAT *)nested_ptr;
							FloatToStr(buf, val, 0, STR_AUTO_PRECISION, ' '); // pico printf %g is broken; use MMBasic's formatter
							MMPrintString(buf);								  // FloatToStr emits '-' for negatives and no leading char for positives
						}
						else if (nsm->type == T_STR)
						{
							MMPrintString("\"");
							int len = *nested_ptr;
							for (int c = 0; c < len; c++)
							{
								char ch[2] = {nested_ptr[c + 1], 0};
								MMPrintString(ch);
							}
							MMPrintString("\"");
						}
						else if (nsm->type == T_STRUCT)
						{
							MMPrintString("(nested struct - use deeper access)");
						}
						MMPrintString("\r\n");
					}
				}
				else if (member_elements == 1)
				{
					// Simple member (not an array)
					sprintf(buf, "  .%s = ", sm->name);
					MMPrintString(buf);

					if (sm->type == T_INT)
					{
						long long int val = *(long long int *)member_ptr;
						IntToStr(buf, val, 10);                         // (newlib nano has no %lld: it printed "ld")
						MMPrintString(buf);
					}
					else if (sm->type == T_NBR)
					{
						MMFLOAT val = *(MMFLOAT *)member_ptr;
						FloatToStr(buf, val, 0, STR_AUTO_PRECISION, ' '); // pico printf %g is broken; use MMBasic's formatter
						MMPrintString(buf);								  // FloatToStr emits '-' for negatives and no leading char for positives
					}
					else if (sm->type == T_STR)
					{
						MMPrintString("\"");
						// String: first byte is length
						int len = *member_ptr;
						for (int c = 0; c < len; c++)
						{
							char ch[2] = {member_ptr[c + 1], 0};
							MMPrintString(ch);
						}
						MMPrintString("\"");
					}
					MMPrintString("\r\n");
				}
				else
				{
					// Array member
					sprintf(buf, "  .%s() = ", sm->name);
					MMPrintString(buf);

					int elem_size;
					if (sm->type == T_STR)
						elem_size = sm->size + 1; // +1 for length byte
					else
						elem_size = sm->size;

					for (int ai = 0; ai < member_elements; ai++)
					{
						char *arr_ptr = member_ptr + (ai * elem_size);

						if (ai > 0)
							MMPrintString(", ");

						if (sm->type == T_INT)
						{
							long long int val = *(long long int *)arr_ptr;
							IntToStr(buf, val, 10);                         // (newlib nano has no %lld: it printed "ld")
							MMPrintString(buf);
						}
						else if (sm->type == T_NBR)
						{
							MMFLOAT val = *(MMFLOAT *)arr_ptr;
							FloatToStr(buf, val, 0, STR_AUTO_PRECISION, ' '); // pico printf %g is broken; use MMBasic's formatter
							MMPrintString(buf);								  // FloatToStr emits '-' for negatives and no leading char for positives
						}
						else if (sm->type == T_STR)
						{
							MMPrintString("\"");
							int len = *arr_ptr;
							for (int c = 0; c < len; c++)
							{
								char ch[2] = {arr_ptr[c + 1], 0};
								MMPrintString(ch);
							}
							MMPrintString("\"");
						}
					}
					MMPrintString("\r\n");
				}
			}
		}
	}
	else if ((p = checkstring(cmdline, (char *)"EXTRACT")) != NULL)
	{
		// STRUCT EXTRACT structarray().membername, destarray()
		// Extracts a single member from each structure in an array into a simple array
		// This allows structure data to be used with commands that expect contiguous arrays
		// (e.g., LINE PLOT, MATH commands)
		int src_idx, dst_idx, struct_type, struct_size;
		char *tp;
		int member_type, member_offset, member_size;
		int src_num_elements, dst_num_elements;
		char *src_base, *dst_base;

		skipspace(p);

		// Get source struct array with member access: structarray().membername
		// findvar will resolve the member access and set g_StructMemberType/Offset/Size
		findvar(p, V_FIND | V_NOFIND_ERR | V_EMPTY_OK);
		src_idx = VarIndex;

		// Check it's a struct array with member access
		if (!(VREC(src_idx)->type & T_STRUCT))
			error("Expected a structure array");
		if (DimIsScalar(RAW_DIM((*VREC(src_idx)), 0)))
			error("Expected a structure array, not a single structure");
		if (!DimIsEnd(RAW_DIM((*VREC(src_idx)), 1)))
			error("Only 1-dimensional structure arrays are supported");
		if (g_StructMemberType == 0)
			error("Expected structarray().membername syntax");

		// Get member info from globals set by findvar
		member_type = g_StructMemberType;
		member_offset = g_StructMemberOffset;
		member_size = g_StructMemberSize;

		// Member cannot be a nested struct
		if (member_type == T_STRUCT)
			error("Cannot extract nested structure member");

		struct_type = (int)VREC(src_idx)->size;
		struct_size = g_structtbl[struct_type]->total_size;

		// Calculate number of source elements
		src_num_elements = DimElements(RAW_DIM((*VREC(src_idx)), 0));
		src_base = VREC(src_idx)->val.s;

		// Skip past structarray().membername to find the comma
		tp = skipvar(p, false);
		skipspace(tp);

		if (*tp != ',')
			error("Expected comma and destination array");
		tp++;
		skipspace(tp);

		// Get destination array with empty ()
		dst_base = findvar(tp, V_FIND | V_NOFIND_ERR | V_EMPTY_OK);
		dst_idx = VarIndex;

		// Check destination is a simple array (not struct)
		if (VREC(dst_idx)->type & T_STRUCT)
			error("Destination must be a simple array, not a structure");
		if (DimIsScalar(RAW_DIM((*VREC(dst_idx)), 0)))
			error("Destination must be an array");
		if (!DimIsEnd(RAW_DIM((*VREC(dst_idx)), 1)))
			error("Destination must be a 1-dimensional array");

		// Calculate destination array size
		dst_num_elements = DimElements(RAW_DIM((*VREC(dst_idx)), 0));

		// Check cardinality matches
		if (dst_num_elements != src_num_elements)
			error("Arrays must have the same size (source=%, dest=%)", src_num_elements, dst_num_elements);

		// Check types match
		int dst_type = VREC(dst_idx)->type & (T_INT | T_NBR | T_STR);
		if (dst_type != member_type)
			error("Type mismatch: structure member and destination array must have same type");

		// For strings, check length matches
		if (member_type == T_STR)
		{
			int dst_str_size = VREC(dst_idx)->size; // max string length for dest array
			if (dst_str_size != member_size)
				error("String length mismatch: member length=%, array length=%", member_size, dst_str_size);
		}

		// Perform the extraction
		if (member_type == T_INT)
		{
			long long int *dst = (long long int *)dst_base;
			for (int i = 0; i < src_num_elements; i++)
			{
				char *src_elem = src_base + (i * struct_size) + member_offset;
				dst[i] = *(long long int *)src_elem;
			}
		}
		else if (member_type == T_NBR)
		{
			MMFLOAT *dst = (MMFLOAT *)dst_base;
			for (int i = 0; i < src_num_elements; i++)
			{
				char *src_elem = src_base + (i * struct_size) + member_offset;
				dst[i] = *(MMFLOAT *)src_elem;
			}
		}
		else if (member_type == T_STR)
		{
			int str_size = member_size + 1; // +1 for length byte
			for (int i = 0; i < src_num_elements; i++)
			{
				char *src_elem = src_base + (i * struct_size) + member_offset;
				char *dst_elem = dst_base + (i * str_size);
				memcpy(dst_elem, src_elem, str_size);
			}
		}
	}
	else if ((p = checkstring(cmdline, (char *)"INSERT")) != NULL)
	{
		// STRUCT INSERT srcarray(), structarray().membername
		// Inserts values from a simple array into the specified member of each structure element
		// This is the reverse of STRUCT EXTRACT
		int src_idx, dst_idx, struct_type, struct_size;
		char *tp;
		int member_type, member_offset, member_size;
		int src_num_elements, dst_num_elements;
		char *src_base, *dst_base;

		skipspace(p);

		// Get source simple array variable with empty ()
		src_base = findvar(p, V_FIND | V_NOFIND_ERR | V_EMPTY_OK);
		src_idx = VarIndex;

		// Check source is a simple array (not struct)
		if (VREC(src_idx)->type & T_STRUCT)
			error("Source must be a simple array, not a structure");
		if (DimIsScalar(RAW_DIM((*VREC(src_idx)), 0)))
			error("Source must be an array");
		if (!DimIsEnd(RAW_DIM((*VREC(src_idx)), 1)))
			error("Source must be a 1-dimensional array");

		// Calculate source array size
		src_num_elements = DimElements(RAW_DIM((*VREC(src_idx)), 0));
		int src_type = VREC(src_idx)->type & (T_INT | T_NBR | T_STR);
		int src_str_size = VREC(src_idx)->size; // for strings

		// Skip past source array to find comma
		tp = skipvar(p, false);
		skipspace(tp);

		if (*tp != ',')
			error("Expected comma after source array");
		tp++;
		skipspace(tp);

		// Get destination struct array with member access: structarray().membername
		// findvar will resolve the member access and set g_StructMemberType/Offset/Size
		dst_base = findvar(tp, V_FIND | V_NOFIND_ERR | V_EMPTY_OK);
		dst_idx = VarIndex;

		// Check it's a struct array with member access
		if (!(VREC(dst_idx)->type & T_STRUCT))
			error("Expected a structure array");
		if (DimIsScalar(RAW_DIM((*VREC(dst_idx)), 0)))
			error("Expected a structure array, not a single structure");
		if (!DimIsEnd(RAW_DIM((*VREC(dst_idx)), 1)))
			error("Only 1-dimensional structure arrays are supported");
		if (g_StructMemberType == 0)
			error("Expected structarray().membername syntax");

		// Get member info from globals set by findvar
		member_type = g_StructMemberType;
		member_offset = g_StructMemberOffset;
		member_size = g_StructMemberSize;

		// Member cannot be a nested struct
		if (member_type == T_STRUCT)
			error("Cannot insert into nested structure member");

		struct_type = (int)VREC(dst_idx)->size;
		struct_size = g_structtbl[struct_type]->total_size;

		// Calculate number of destination elements
		dst_num_elements = DimElements(RAW_DIM((*VREC(dst_idx)), 0));

		// Use dst_base from the struct variable, not from member resolution
		dst_base = VREC(dst_idx)->val.s;

		// Check cardinality matches
		if (src_num_elements != dst_num_elements)
			error("Arrays must have the same size (source=%, dest=%)", src_num_elements, dst_num_elements);

		// Check types match
		if (src_type != member_type)
			error("Type mismatch: source array and structure member must have same type");

		// For strings, check length matches
		if (member_type == T_STR)
		{
			if (src_str_size != member_size)
				error("String length mismatch: array length=%, member length=%", src_str_size, member_size);
		}

		// Perform the insertion
		if (member_type == T_INT)
		{
			long long int *src = (long long int *)src_base;
			for (int i = 0; i < dst_num_elements; i++)
			{
				char *dst_elem = dst_base + (i * struct_size) + member_offset;
				*(long long int *)dst_elem = src[i];
			}
		}
		else if (member_type == T_NBR)
		{
			MMFLOAT *src = (MMFLOAT *)src_base;
			for (int i = 0; i < dst_num_elements; i++)
			{
				char *dst_elem = dst_base + (i * struct_size) + member_offset;
				*(MMFLOAT *)dst_elem = src[i];
			}
		}
		else if (member_type == T_STR)
		{
			int str_size = member_size + 1; // +1 for length byte
			for (int i = 0; i < dst_num_elements; i++)
			{
				char *src_elem = src_base + (i * str_size);
				char *dst_elem = dst_base + (i * struct_size) + member_offset;
				memcpy(dst_elem, src_elem, str_size);
			}
		}
	}
	else
	{
		error("Unknown STRUCT subcommand");
	}
}


// STRUCT(FIND array(), membername$, value [, start])
// General structure function with subfunctions
// FIND: Searches a struct array for an element where the specified member equals the value
//       Returns the index of the first match (starting from 'start'), or -1 if not found
//       Optional 'start' parameter allows iteration through multiple matches
void fun_struct(void)
{
	char *p;

	// Check for FIND subfunction: STRUCT(FIND array().member, value [, start] [, size])
	// argc==3: array().member, value (simple match)
	// argc==5: array().member, value, start (simple match with start index)
	// argc==7: array().member, value [, start], size (regex match, start optional)
	if ((p = checkstring(ep, (char *)"FIND")) != NULL)
	{
		char *member_base;
		int var_idx, struct_type, struct_size;
		int member_type, member_offset, member_size;
		int num_elements, i, start_idx;
		int use_regex = 0;
		void *size_var = NULL;
		int64_t *size_var_int = NULL;
		MMFLOAT *size_var_float = NULL;

		// Parse remaining arguments: array().member, value [, start] [, size]
		getargs(&p, 7, ","); // array().member, value [, start] [, size] = up to 7 tokens
		if (argc != 3 && argc != 5 && argc != 7)
			error("Syntax: STRUCT(FIND array().member, value [, start] [, size])");

		// Determine if regex mode (argc==7 means regex with size variable)
		if (argc == 7)
		{
			use_regex = 1;
		}

		// Get the struct array member variable (argv[0])
		// This uses findvar which will populate g_StructMemberOffset and g_StructMemberSize
		member_base = findvar(argv[0], V_FIND | V_NOFIND_ERR | V_EMPTY_OK);
		var_idx = VarIndex;

		if (!(VREC(var_idx)->type & T_STRUCT))
			error("Expected a structure array");

		if (DimIsScalar(RAW_DIM((*VREC(var_idx)), 0)))
			error("Expected an array of structures");

		// Check that a member was specified (findvar sets g_StructMemberOffset)
		if (g_StructMemberOffset < 0)
			error("Must specify a member (e.g., array().membername)");

		struct_type = (int)VREC(var_idx)->size;
		struct_size = g_structtbl[struct_type]->total_size;
		member_offset = g_StructMemberOffset;
		member_size = g_StructMemberSize;

		// Get array base pointer (member_base points to member in element 0)
		char *array_base = member_base - member_offset;

		// Calculate number of elements
		num_elements = 1;
		for (int d = 0; d < MAXDIM && !DimIsEnd(RAW_DIM((*VREC(var_idx)), d)); d++)
		{
			num_elements *= DimElements(RAW_DIM((*VREC(var_idx)), d));
		}

		// Determine member type from member_size and structure definition
		// member_size > 8 means string (includes length byte)
		if (member_size > 8)
		{
			member_type = T_STR;
		}
		else
		{
			// Check if the member is integer or float by looking at the structure definition
			struct s_structdef *sd = g_structtbl[struct_type];
			member_type = T_NBR; // default
			for (int m = 0; m < sd->num_members; m++)
			{
				if (sd->members[m].offset == member_offset)
				{
					member_type = sd->members[m].type & (T_INT | T_NBR | T_STR);
					break;
				}
			}
		}

		// Get search value and determine its type (argv[2])
		MMFLOAT f;
		long long int i64;
		char *s = NULL;
		int t = T_NOTYPE;
		evaluate(argv[2], &f, &i64, &s, &t, false);

		// Get optional start index (argv[4] if provided and non-empty)
		// For regex mode (argc==7): start is in argv[4] (optional), size var is in argv[6]
		// For simple mode (argc==5): start is in argv[4]
		start_idx = 0;
		if (argc >= 5 && *argv[4])
		{
			start_idx = getint(argv[4], OptionBase, OptionBase + num_elements) - OptionBase;
			// If start is past end of array, return -1 immediately
			if (start_idx >= num_elements)
			{
				targ = T_INT;
				iret = -1;
				return;
			}
		}

		// For regex mode, get the size variable (argv[6])
		if (use_regex)
		{
			if (member_type != T_STR)
				error("Regex search only works with string members");
			if (!(t & T_STR))
				error("Regex pattern must be a string");

			int size_vtype;
			size_var = findvar(argv[6], V_FIND);
			size_vtype = VREC(VarIndex)->type;
			if (g_StructMemberType != 0)
				size_vtype = g_StructMemberType;
			if (!(size_vtype & (T_NBR | T_INT)))
				error("Size variable must be numeric");
			if (size_vtype & T_INT)
				size_var_int = size_var;
			else
				size_var_float = size_var;
		}

		// Search the array starting from start_idx
		targ = T_INT;
		for (i = start_idx; i < num_elements; i++)
		{
			char *elem_ptr = array_base + (i * struct_size);
			char *member_ptr = elem_ptr + member_offset;

			if (member_type == T_INT)
			{
				long long int val = *(long long int *)member_ptr;
				long long int search_val = 0;
				if (t & T_INT)
					search_val = i64;
				else if (t & T_NBR)
					search_val = (long long int)f;
				else
					error("Type mismatch: expected numeric value");

				if (val == search_val)
				{
					iret = i + OptionBase;
					return;
				}
			}
			else if (member_type == T_NBR)
			{
				MMFLOAT val = *(MMFLOAT *)member_ptr;
				MMFLOAT search_val = 0;
				if (t & T_NBR)
					search_val = f;
				else if (t & T_INT)
					search_val = (MMFLOAT)i64;
				else
					error("Type mismatch: expected numeric value");

				if (val == search_val)
				{
					iret = i + OptionBase;
					return;
				}
			}
			else if (member_type == T_STR)
			{
				char *val = member_ptr; // Points to length byte

				if (use_regex)
				{
					// Regex search mode
					int match_length;
					char *text_cstr = GetTempMemory(STRINGSIZE);
					char *pattern_cstr = GetTempMemory(STRINGSIZE);

					// Convert MMBasic string to C string
					memcpy(text_cstr, val + 1, *val);
					text_cstr[(unsigned char)*val] = '\0';

					// Convert pattern (s is MMBasic string)
					memcpy(pattern_cstr, s + 1, *s);
					pattern_cstr[(unsigned char)*s] = '\0';

					int tmp = OptionEscape;
					OptionEscape = 0;
					int match_idx = re_match(pattern_cstr, text_cstr, &match_length);
					OptionEscape = tmp;

					if (match_idx != -1)
					{
						// Found a match - update size variable with match length
						if (size_var_float)
							*size_var_float = (MMFLOAT)match_length;
						else
							*size_var_int = (int64_t)match_length;
						iret = i + OptionBase;
						return;
					}
				}
				else
				{
					// Simple string match
					if (!(t & T_STR))
						error("Type mismatch: expected string value");

					// Compare strings (MMBasic string format: first byte is length)
					if (*val == *s && memcmp(val + 1, s + 1, *s) == 0)
					{
						iret = i + OptionBase;
						return;
					}
				}
			}
		}

		// Not found - set size to 0 for regex mode
		if (use_regex)
		{
			if (size_var_float)
				*size_var_float = 0.0;
			else
				*size_var_int = 0;
		}
		iret = -1;
		return;
	}

	// Check for OFFSET subfunction: STRUCT(OFFSET typename$, element$)
	if ((p = checkstring(ep, (char *)"OFFSET")) != NULL)
	{
		char *typename_str, *element_str;
		int i, member_type, member_offset, member_size;

		// Parse remaining arguments: typename$, element$
		getargs(&p, 3, ","); // typename$, element$ = 3 tokens
		if (argc != 3)
			error("Syntax: STRUCT(OFFSET typename$, element$)", "$", "$");   // ($ is error()'s code for a string: the PicoMite's #111)

		// Get the structure type name (argv[0])
		typename_str = getstring(argv[0]);

		// Get the element/member name (argv[2])
		element_str = getstring(argv[2]);

		// Search for the structure type by name
		for (i = 0; i < g_structcnt; i++)
		{
			if (g_structtbl[i] != NULL &&
				strlen((char *)g_structtbl[i]->name) == *typename_str &&
				strncasecmp((char *)g_structtbl[i]->name, (char *)(typename_str + 1), *typename_str) == 0)
			{
				// Found the structure type, now find the member
				int member_idx = FindStructMember(i, element_str + 1, &member_type, &member_offset, &member_size, NULL);
				if (member_idx < 0)
					error("Member not found in structure");
				targ = T_INT;
				iret = member_offset;
				return;
			}
		}

		error("Structure type not found");
	}

	// Check for SIZEOF subfunction: STRUCT(SIZEOF typename$)
	if ((p = checkstring(ep, (char *)"SIZEOF")) != NULL)
	{
		char *typename_str;
		int i;

		// Parse remaining argument: typename$
		getargs(&p, 1, ","); // typename$ = 1 token
		if (argc != 1)
			error("Syntax: STRUCT(SIZEOF typename$)", "$");   // ($ is error()'s code for a string: the PicoMite's #111)

		// Get the structure type name (argv[0])
		typename_str = getstring(argv[0]);

		// Search for the structure type by name
		for (i = 0; i < g_structcnt; i++)
		{
			if (g_structtbl[i] != NULL &&
				strlen((char *)g_structtbl[i]->name) == *typename_str &&
				strncasecmp((char *)g_structtbl[i]->name, (char *)(typename_str + 1), *typename_str) == 0)
			{
				targ = T_INT;
				iret = g_structtbl[i]->total_size;
				return;
			}
		}

		error("Structure type not found");
	}

	// Check for TYPE subfunction: STRUCT(TYPE typename$, element$)
	// Returns T_INT, T_NBR or T_STR for the base type of the element
	if ((p = checkstring(ep, (char *)"TYPE")) != NULL)
	{
		char *typename_str, *element_str;
		int i, member_type, member_offset, member_size;

		// Parse remaining arguments: typename$, element$
		getargs(&p, 3, ","); // typename$, element$ = 3 tokens
		if (argc != 3)
			error("Syntax: STRUCT(TYPE typename$, element$)", "$", "$");   // ($ is error()'s code for a string: the PicoMite's #111)

		// Get the structure type name (argv[0])
		typename_str = getstring(argv[0]);

		// Get the element/member name (argv[2])
		element_str = getstring(argv[2]);

		// Search for the structure type by name
		for (i = 0; i < g_structcnt; i++)
		{
			if (g_structtbl[i] != NULL &&
				strlen((char *)g_structtbl[i]->name) == *typename_str &&
				strncasecmp((char *)g_structtbl[i]->name, (char *)(typename_str + 1), *typename_str) == 0)
			{
				// Found the structure type, now find the member
				int member_idx = FindStructMember(i, element_str + 1, &member_type, &member_offset, &member_size, NULL);
				if (member_idx < 0)
					error("Member not found in structure");
				targ = T_INT;
				// Return only the base type (T_INT, T_NBR or T_STR)
				iret = member_type & (T_INT | T_NBR | T_STR);
				return;
			}
		}

		error("Structure type not found");
	}

	error("Unknown STRUCT subfunction");
}
