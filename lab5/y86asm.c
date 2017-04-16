#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "y86asm.h"

line_t *y86bin_listhead = NULL;   /* the head of y86 binary code line list*/
line_t *y86bin_listtail = NULL;   /* the tail of y86 binary code line list*/
int y86asm_lineno = 0; /* the current line number of y86 assemble code */

#define err_print(_s, _a ...) do { \
  if (y86asm_lineno < 0) \
    fprintf(stderr, "[--]: "_s"\n", ## _a); \
  else \
    fprintf(stderr, "[L%d]: "_s"\n", y86asm_lineno, ## _a); \
} while (0);

int vmaddr = 0;    /* vm addr */

/* register table */
reg_t reg_table[REG_CNT] = {
    {"%eax", REG_EAX},
    {"%ecx", REG_ECX},
    {"%edx", REG_EDX},
    {"%ebx", REG_EBX},
    {"%esp", REG_ESP},
    {"%ebp", REG_EBP},
    {"%esi", REG_ESI},
    {"%edi", REG_EDI},
};

regid_t find_register(char *name) {
    return REG_ERR;
}

/* instruction set */
instr_t instr_set[] = {
    {"nop", 3,   HPACK(I_NOP, F_NONE), 1 },
    {"halt", 4,  HPACK(I_HALT, F_NONE), 1 },
    {"rrmovl", 6,HPACK(I_RRMOVL, F_NONE), 2 },
    {"cmovle", 6,HPACK(I_RRMOVL, C_LE), 2 },
    {"cmovl", 5, HPACK(I_RRMOVL, C_L), 2 },
    {"cmove", 5, HPACK(I_RRMOVL, C_E), 2 },
    {"cmovne", 6,HPACK(I_RRMOVL, C_NE), 2 },
    {"cmovge", 6,HPACK(I_RRMOVL, C_GE), 2 },
    {"cmovg", 5, HPACK(I_RRMOVL, C_G), 2 },
    {"irmovl", 6,HPACK(I_IRMOVL, F_NONE), 6 },
    {"rmmovl", 6,HPACK(I_RMMOVL, F_NONE), 6 },
    {"mrmovl", 6,HPACK(I_MRMOVL, F_NONE), 6 },
    {"addl", 4,  HPACK(I_ALU, A_ADD), 2 },
    {"subl", 4,  HPACK(I_ALU, A_SUB), 2 },
    {"andl", 4,  HPACK(I_ALU, A_AND), 2 },
    {"xorl", 4,  HPACK(I_ALU, A_XOR), 2 },
    {"jmp", 3,   HPACK(I_JMP, C_YES), 5 },
    {"jle", 3,   HPACK(I_JMP, C_LE), 5 },
    {"jl", 2,    HPACK(I_JMP, C_L), 5 },
    {"je", 2,    HPACK(I_JMP, C_E), 5 },
    {"jne", 3,   HPACK(I_JMP, C_NE), 5 },
    {"jge", 3,   HPACK(I_JMP, C_GE), 5 },
    {"jg", 2,    HPACK(I_JMP, C_G), 5 },
    {"call", 4,  HPACK(I_CALL, F_NONE), 5 },
    {"ret", 3,   HPACK(I_RET, F_NONE), 1 },
    {"pushl", 5, HPACK(I_PUSHL, F_NONE), 2 },
    {"popl", 4,  HPACK(I_POPL, F_NONE),  2 },
    {".byte", 5, HPACK(I_DIRECTIVE, D_DATA), 1 },
    {".word", 5, HPACK(I_DIRECTIVE, D_DATA), 2 },
    {".long", 5, HPACK(I_DIRECTIVE, D_DATA), 4 },
    {".pos", 4,  HPACK(I_DIRECTIVE, D_POS), 0 },
    {".align", 6,HPACK(I_DIRECTIVE, D_ALIGN), 0 },
    {NULL, 1,    0   , 0 } //end
};

instr_t *find_instr(char *name) {
	instr_t *pi = instr_set;
	while (pi->name != NULL) {
		if (strcmp(pi->name, name) == 0)
			return pi;
		pi ++;
	}
    return NULL;
}

/* symbol table (don't forget to init and finit it) */
symbol_t *symtab = NULL;
/*
 * find_symbol: scan table to find the symbol
 * args
 *     name: the name of symbol
 *
 * return
 *     symbol_t: the 'name' symbol
 *     NULL: not exist
 */
symbol_t *find_symbol(char *name) {
	symbol_t *ps = symtab->next;
	while (ps != NULL) {
		if (strcmp(ps->name, name) == 0)
			return ps;
		ps = ps->next;
	}
    return NULL;
}

/*
 * add_symbol: add a new symbol to the symbol table
 * args
 *     name: the name of symbol
 *
 * return
 *     0: success
 *     -1: error, the symbol has exist
 */
int add_symbol(char *name) {   
    /* check duplicate */
    if (find_symbol(name) != NULL) {
    	err_print("Dup symbol:%s", name);
    	return -1;
    }

    /* create new symbol_t (don't forget to free it)*/
    symbol_t *new_symbol = (symbol_t *)malloc(sizeof(symbol_t));
    new_symbol->name = name;
    new_symbol->addr = vmaddr;

    /* add the new symbol_t to symbol table */
    symbol_t *ps = symtab;
	while (ps->next != NULL)
	   	ps = ps->next;
    (*ps).next = new_symbol;
    return 0;
}

/* relocation table (don't forget to init and finit it) */
reloc_t *reltab = NULL;

/*
 * add_reloc: add a new relocation to the relocation table
 * args
 *     name: the name of symbol
 *
 * return
 *     0: success
 *     -1: error, the symbol has exist
 */
void add_reloc(char *name, bin_t *bin) {
    /* create new reloc_t (don't forget to free it)*/
    reloc_t *new_reloc = (reloc_t *)malloc(sizeof(reloc_t));
    new_reloc->y86bin = bin;
    new_reloc->name = name;
    /* add the new reloc_t to relocation table */
    reloc_t *pr = reltab;
    while (pr->next != NULL)
    	pr = pr->next;
    (*pr).next = new_reloc;
}


/* macro for parsing y86 assembly code */
#define IS_DIGIT(s) ((*(s)>='0' && *(s)<='9') || *(s)=='-' || *(s)=='+')
#define IS_LETTER(s) ((*(s)>='a' && *(s)<='z') || (*(s)>='A' && *(s)<='Z'))
#define IS_COMMENT(s) (*(s)=='#')
#define IS_REG(s) (*(s)=='%')
#define IS_IMM(s) (*(s)=='$')

#define IS_BLANK(s) (*(s)==' ' || *(s)=='\t')
#define IS_END(s) (*(s)=='\0')

#define SKIP_BLANK(s) do {  \
  while(!IS_END(s) && IS_BLANK(s))  \
    (s)++;    \
} while(0);

/* return value from different parse_xxx function */
typedef enum { PARSE_ERR=-1, PARSE_REG, PARSE_DIGIT, PARSE_SYMBOL, 
    PARSE_MEM, PARSE_DELIM, PARSE_INSTR, PARSE_LABEL} parse_t;

/*
 * parse_instr: parse an expected data token (e.g., 'rrmovl')
 * args
 *     ptr: point to the start of string
 *     inst: point to the inst_t within instr_set
 *
 * return
 *     PARSE_INSTR: success, move 'ptr' to the first char after token,
 *                            and store the pointer of the instruction to 'inst'
 *     PARSE_ERR: error, the value of 'ptr' and 'inst' are undefined
 */
parse_t parse_instr(char **ptr, instr_t **inst) {
    /* skip the blank */
	SKIP_BLANK(*ptr)
	if (IS_END(*ptr))
		return PARSE_ERR;
    /* find_instr and check end */
    char *now = *ptr;
    if (**ptr == ':')//skip :
    	*ptr = *ptr + 1;
    int cnt = 0;
	while ((!IS_BLANK(now)) && (!IS_END(now))) {
		cnt ++;
		now ++;
	}
	char ins[cnt + 1];
	memset(ins, '\0', sizeof(ins));
	strncpy(ins, *ptr, cnt);

    /* set 'ptr' and 'inst' */
	*inst = find_instr(ins);
	*ptr = now;
	if (*inst == NULL)
    	return PARSE_ERR;
    else
    	return PARSE_INSTR;
}

/*
 * parse_delim: parse an expected delimiter token (e.g., ',')
 * args
 *     ptr: point to the start of string
 *
 * return
 *     PARSE_DELIM: success, move 'ptr' to the first char after token
 *     PARSE_ERR: error, the value of 'ptr' and 'delim' are undefined
 */
parse_t parse_delim(char **ptr) {
    /* skip the blank and check */
	SKIP_BLANK(*ptr)
	if (IS_END(*ptr))
		return PARSE_ERR;
	if (**ptr == ',') {
		*ptr = *ptr + 1;
		return PARSE_DELIM;
	}
    /* set 'ptr' */
	err_print("Invalid ','");
    return PARSE_ERR;
}

/*
 * parse_reg: parse an expected register token (e.g., '%eax')
 * args
 *     ptr: point to the start of string
 *     regid: point to the regid of register
 *
 * return
 *     PARSE_REG: success, move 'ptr' to the first char after token, 
 *                         and store the regid to 'regid'
 *     PARSE_ERR: error, the value of 'ptr' and 'regid' are undefined
 */
parse_t parse_reg(char **ptr, regid_t *regid) {
    /* skip the blank and check */
	SKIP_BLANK(*ptr)
	if (IS_END(*ptr))
		return PARSE_ERR;
    /* find register */
    char reg[4];
    memset(reg, '\0', 4);
	if (IS_REG(*ptr)) {
		*ptr = *ptr + 1;
		strncpy(reg, *ptr, 3);
	}
	else
		return PARSE_ERR;

    /* set 'ptr' and 'regid' */
    *ptr = *ptr + 3;
    if (strcmp(reg, "eax") == 0) {
    	*regid = REG_EAX;
    	return PARSE_REG;
    }
    else if (strcmp(reg, "ebx") == 0) {
    	*regid = REG_EBX;
    	return PARSE_REG;
    }
   	else if (strcmp(reg, "ecx") == 0) {
   		*regid = REG_ECX;
   		return PARSE_REG;
   	}
   	else if (strcmp(reg, "edx") == 0) {
   		*regid = REG_EDX;
   		return PARSE_REG;
   	}
   	else if (strcmp(reg, "esp") == 0) {
   		*regid = REG_ESP;
   		return PARSE_REG;
   	}
   	else if (strcmp(reg, "ebp") == 0) {
   		*regid = REG_EBP;
   		return PARSE_REG;
   	}
   	else if (strcmp(reg, "esi") == 0) {
   		*regid = REG_ESI;
   		return PARSE_REG;
   	}
   	else if (strcmp(reg, "edi") == 0) {
   		*regid = REG_EDI;
   		return PARSE_REG;
   	}
   	else {
   		err_print("Invalid REG");
   		return PARSE_ERR;
   	}
}

/*
 * parse_symbol: parse an expected symbol token (e.g., 'Main')
 * args
 *     ptr: point to the start of string
 *     name: point to the name of symbol (should be allocated in this function)
 *
 * return
 *     PARSE_SYMBOL: success, move 'ptr' to the first char after token,
 *                               and allocate and store name to 'name'
 *     PARSE_ERR: error, the value of 'ptr' and 'name' are undefined
 */
parse_t parse_symbol(char **ptr, char **name) {
    /* skip the blank and check */

    SKIP_BLANK(*ptr)
    if (IS_END(*ptr))
		return PARSE_ERR;

    /* allocate name and copy to it */
	char *now = *ptr;
	int cnt = 0;
	if (IS_DIGIT(now)) {
		err_print("Invalid DEST");
		return PARSE_ERR;
	}
	while ((!IS_BLANK(now)) && (!IS_END(now)) && ((IS_LETTER(now)) || (IS_DIGIT(now)))) {
		cnt ++;
		now ++;
	}
	/* set 'ptr' and 'name' */
	*name = (char *)malloc(cnt + 1);
	memset(*name, '\0', cnt + 1);
	strncpy(*name, *ptr, cnt);
	*ptr = now;
	return PARSE_SYMBOL;
	
}

/*
 * parse_digit: parse an expected digit token (e.g., '0x100')
 * args
 *     ptr: point to the start of string
 *     value: point to the value of digit
 *
 * return
 *     PARSE_DIGIT: success, move 'ptr' to the first char after token
 *                            and store the value of digit to 'value'
 *     PARSE_ERR: error, the value of 'ptr' and 'value' are undefined
 */
parse_t parse_digit(char **ptr, long *value) {
    /* skip the blank and check */
	SKIP_BLANK(*ptr)
    if (IS_END(*ptr))
		return PARSE_ERR;
    /* calculate the digit, (NOTE: see strtoll()) */
    if (!IS_DIGIT(*ptr)) {
    	err_print("Invalid Immediate");
    	return PARSE_ERR;
    }
	long long digit = strtoll(*ptr, NULL, 0);
    /* set 'ptr' and 'value' */
	*value = digit;
	while (IS_DIGIT(*ptr) | (**ptr == 'x') | (**ptr == '-') | (**ptr == 'A') | (**ptr == 'B') | (**ptr == 'C') | (**ptr == 'D')
		| (**ptr == 'E') | (**ptr == 'F') | (**ptr == 'a') | (**ptr == 'b') | (**ptr == 'c') | (**ptr == 'd') | (**ptr == 'e') 
		| (**ptr == 'f'))
		*ptr = *ptr + 1;
    return PARSE_DIGIT;
}

/*
 * parse_imm: parse an expected immediate token (e.g., '$0x100' or 'STACK')
 * args
 *     ptr: point to the start of string
 *     name: point to the name of symbol (should be allocated in this function)
 *     value: point to the value of digit
 *
 * return
 *     PARSE_DIGIT: success, the immediate token is a digit,
 *                            move 'ptr' to the first char after token,
 *                            and store the value of digit to 'value'
 *     PARSE_SYMBOL: success, the immediate token is a symbol,
 *                            move 'ptr' to the first char after token,
 *                            and allocate and store name to 'name' 
 *     PARSE_ERR: error, the value of 'ptr', 'name' and 'value' are undefined
 */
parse_t parse_imm(char **ptr, char **name, long *value) {
    /* skip the blank and check */
	SKIP_BLANK(*ptr)
    if (IS_END(*ptr))
		return PARSE_ERR;
    /* if IS_IMM, then parse the digit */
	if (IS_IMM(*ptr)) {
		*ptr = *ptr + 1;
		return parse_digit(ptr, value);
	}
    /* if IS_LETTER, then parse the symbol */
    if (IS_LETTER(*ptr))
    	return parse_symbol(ptr, name);
    /* set 'ptr' and 'name' or 'value' */
    //complete in the 2 functions above
    
    return PARSE_ERR;
}

/*
 * parse_mem: parse an expected memory token (e.g., '8(%ebp)')
 * args
 *     ptr: point to the start of string
 *     value: point to the value of digit
 *     regid: point to the regid of register
 *
 * return
 *     PARSE_MEM: success, move 'ptr' to the first char after token,
 *                          and store the value of digit to 'value',
 *                          and store the regid to 'regid'
 *     PARSE_ERR: error, the value of 'ptr', 'value' and 'regid' are undefined
 */
parse_t parse_mem(char **ptr, long *value, regid_t *regid) {
    /* skip the blank and check */
	SKIP_BLANK(*ptr)
    if (IS_END(*ptr))
		return PARSE_ERR;
    /* calculate the digit and register, (ex: (%ebp) or 8(%ebp)) */
	if (**ptr == '(') {
		*ptr = *ptr + 1;
		if (parse_reg(ptr, regid) == PARSE_REG) {
			if (**ptr == ')')
				*ptr = *ptr + 1;
			else {
				err_print("Invalid MEM");
				return PARSE_ERR;
			}
			*value = 0;
			return PARSE_MEM;
		}
		else
			return PARSE_ERR;
	}
	if (**ptr != '(') {
		if ((parse_digit(ptr, value) == PARSE_DIGIT) && (**ptr == '(')) {
			*ptr = *ptr + 1;
			if (parse_reg(ptr, regid) == PARSE_REG) {
				if (**ptr == ')')
				*ptr = *ptr + 1;
				else {
					err_print("Invalid MEM");
					return PARSE_ERR;
				}
				return PARSE_MEM;
			}
		}
		else {
			return PARSE_ERR;
		}

	}
    /* set 'ptr', 'value' and 'regid' */
	//complete in the 2 functions above
    return PARSE_ERR;
}

/*
 * parse_data: parse an expected data token (e.g., '0x100' or 'array')
 * args
 *     ptr: point to the start of string
 *     name: point to the name of symbol (should be allocated in this function)
 *     value: point to the value of digit
 *
 * return
 *     PARSE_DIGIT: success, data token is a digit,
 *                            and move 'ptr' to the first char after token,
 *                            and store the value of digit to 'value'
 *     PARSE_SYMBOL: success, data token is a symbol,
 *                            and move 'ptr' to the first char after token,
 *                            and allocate and store name to 'name' 
 *     PARSE_ERR: error, the value of 'ptr', 'name' and 'value' are undefined
 */
parse_t parse_data(char **ptr, char **name, long *value) {
    /* skip the blank and check */
	SKIP_BLANK(*ptr)
    if (IS_END(*ptr))
		return PARSE_ERR;
    /* if IS_DIGIT, then parse the digit */
	if (IS_DIGIT(*ptr))
		return parse_digit(ptr, value);
    /* if IS_LETTER, then parse the symbol */
	if (IS_LETTER(*ptr))
		return parse_symbol(ptr, name);
    /* set 'ptr', 'name' and 'value' */
    //complete in the 2 functions above
	
    return PARSE_ERR;
}

/*
 * parse_label: parse an expected label token (e.g., 'Loop:')
 * args
 *     ptr: point to the start of string
 *     name: point to the name of symbol (should be allocated in this function)
 *
 * return
 *     PARSE_LABEL: success, move 'ptr' to the first char after token
 *                            and allocate and store name to 'name'
 *     PARSE_ERR: error, the value of 'ptr' is undefined
 */
parse_t parse_label(char **ptr, char **name) {
    /* skip the blank and check */
	SKIP_BLANK(*ptr)
    if (IS_END(*ptr))
		return PARSE_ERR;
    /* allocate name and copy to it */
    bool_t is_label = FALSE;
    char *now = *ptr;
	while (!IS_END(now)) {
		if (*now == ':') {
			is_label = TRUE;
			break;
		}
		else
			now ++;
	}
	if (is_label) {
		int cnt = 0;
		now = *ptr;
		while ((*now != ':') && (!IS_END(now))) {
			cnt ++;
			now ++;
		}
	    /* set 'ptr' and 'name' */
	    *name = (char *)malloc(cnt + 1);
	    memset(*name, '\0', cnt + 1);
	    strncpy(*name, *ptr, cnt);
	    *ptr = now + 1;
	    return PARSE_LABEL;
	}
	
	return PARSE_ERR;
}

#define PUT_DIGIT line->y86bin.codes[2] = HPACK((value >> 4) & 0xF, (value >> 0) & 0xF); \
line->y86bin.codes[3] = HPACK((value >> 12) & 0xF, (value >> 8) & 0xF); \
line->y86bin.codes[4] = HPACK((value >> 20) & 0xF, (value >> 16) & 0xF); \
line->y86bin.codes[5] = HPACK((value >> 28) & 0xF, (value >> 24) & 0xF);


/*
 * parse_line: parse a line of y86 code (e.g., 'Loop: mrmovl (%ecx), %esi')
 * (you could combine above parse_xxx functions to do it)
 * args
 *     line: point to a line_t data with a line of y86 assembly code
 *
 * return
 *     PARSE_XXX: success, fill line_t with assembled y86 code
 *     PARSE_ERR: error, try to print err information (e.g., instr type and line number)
 */
type_t parse_line(line_t *line) {

/* when finish parse an instruction or lable, we still need to continue check 
* e.g., 
*  Loop: mrmovl (%ebp), %ecx
*           call SUM  #invoke SUM function */

	char *now = line->y86asm;
	char *l_name = NULL;//name for label
	char *s_name = NULL;//name for symbol
	long value = 0;
	regid_t regidA = REG_NONE;
	regid_t regidB = REG_NONE;
	instr_t *inst = instr_set;

    /* skip blank and check IS_END */
    SKIP_BLANK(now)
    if (IS_END(now))
    	return TYPE_INS;
    /* is a comment ? */
	if (IS_COMMENT(now))
		return TYPE_COMM;
    /* is a label ? */
	if (parse_label(&now, &l_name) == PARSE_LABEL) {
		if (add_symbol(l_name) == -1) {
			return TYPE_ERR;
		}
		line->y86bin.addr = vmaddr;
		SKIP_BLANK(now)
		if (IS_COMMENT(now)) {
			line->type = TYPE_INS;
			line->y86bin.bytes = 0;
			return line->type;
		}
			
	}

    /* is an instruction ? */
	if (parse_instr(&now, &inst) == PARSE_INSTR) {
		/* set type and y86bin */
		line->type = TYPE_INS;
		line->y86bin.addr = vmaddr;
		line->y86bin.bytes = inst->bytes;
		if (*inst->name != '.')
			line->y86bin.codes[0] = inst->code;

		/* update vmaddr */    
		vmaddr += inst->bytes;

		/* parse the rest of instruction according to the itype */
		if ((strcmp("rrmovl", inst->name) == 0) | (strcmp("cmovle", inst->name) == 0) | (strcmp("cmovl", inst->name) == 0)
			| (strcmp("cmove", inst->name) == 0) | (strcmp("cmovne", inst->name) == 0) | (strcmp("cmovge", inst->name) == 0)
			| (strcmp("cmovg", inst->name) == 0) | (strcmp("addl", inst->name) == 0) | (strcmp("subl", inst->name) == 0)
			| (strcmp("andl", inst->name) == 0) | (strcmp("xorl", inst->name) == 0)) {
			if (parse_reg(&now, &regidA) != PARSE_REG)
				return TYPE_ERR; 
			if (parse_delim(&now) != PARSE_DELIM)
				return TYPE_ERR;
			if (parse_reg(&now, &regidB) != PARSE_REG)
				return TYPE_ERR;
			line->y86bin.codes[1] = HPACK(regidA, regidB);
		}

		if (strcmp("irmovl", inst->name) == 0) {
			parse_t imm_stus = parse_imm(&now, &s_name, &value);
			if (imm_stus == PARSE_ERR)
				return TYPE_ERR;
			if (parse_delim(&now) != PARSE_DELIM)
				return TYPE_ERR; 
			if (parse_reg(&now, &regidB) != PARSE_REG)
				return TYPE_ERR;
			line->y86bin.codes[1] = HPACK(REG_NONE, regidB);
			if (imm_stus == PARSE_DIGIT){
				PUT_DIGIT
			}
			else {
				symbol_t *ps = find_symbol(s_name);
				if (ps != NULL) {
					value = ps->addr;
					PUT_DIGIT
				}
				else
					add_reloc(s_name, &line->y86bin);
			}

		}

		if (strcmp("rmmovl", inst->name) == 0) {
			if (parse_reg(&now, &regidA) != PARSE_REG)
				return TYPE_ERR;
			if (parse_delim(&now) != PARSE_DELIM)
				return TYPE_ERR;
			parse_mem(&now, &value, &regidB);
			line->y86bin.codes[1] = HPACK(regidA, regidB);
			PUT_DIGIT
		}

		if (strcmp("mrmovl", inst->name) == 0) {
			if (parse_mem(&now, &value, &regidB) != PARSE_MEM)
				return TYPE_ERR;
			if (parse_delim(&now) != PARSE_DELIM)
				return TYPE_ERR;
			if (parse_reg(&now, &regidA) != PARSE_REG)
				return TYPE_ERR;
			line->y86bin.codes[1] = HPACK(regidA, regidB);
			PUT_DIGIT
		}

		if ((strcmp("jmp", inst->name) == 0) | (strcmp("jle", inst->name) == 0) | (strcmp("jl", inst->name) == 0)
			| (strcmp("je", inst->name) == 0) | (strcmp("jne", inst->name) == 0) | (strcmp("jge", inst->name) == 0)
			| (strcmp("jg", inst->name) == 0) | (strcmp("call", inst->name) == 0)) {
			if (parse_symbol(&now, &s_name) != PARSE_SYMBOL)
				return TYPE_ERR;
			symbol_t *ps = find_symbol(s_name);
			if (ps != NULL) {
				value = ps->addr;
				line->y86bin.codes[1] = HPACK((value >> 4) & 0xF, (value >> 0) & 0xF);
				line->y86bin.codes[2] = HPACK((value >> 12) & 0xF, (value >> 8) & 0xF);
				line->y86bin.codes[3] = HPACK((value >> 20) & 0xF, (value >> 16) & 0xF);
				line->y86bin.codes[4] = HPACK((value >> 28) & 0xF, (value >> 24) & 0xF);
			}
			else
				add_reloc(s_name, &line->y86bin);
		}

		if ((strcmp("pushl", inst->name) == 0) | (strcmp("popl", inst->name) == 0)) {
			if (parse_reg(&now, &regidA) != PARSE_REG)
				return TYPE_ERR;
			line->y86bin.codes[1] = HPACK(regidA, REG_NONE);
		}

		//directive orders:
		if (strcmp(".pos", inst->name) == 0) {
			parse_digit(&now, &value);
			vmaddr = value;
			line->y86bin.addr = vmaddr;

		}

		if (strcmp(".align", inst->name) == 0) {
			parse_digit(&now, &value);
			while (vmaddr % value != 0)
				vmaddr ++;
			line->y86bin.addr = vmaddr;
		}

		if (strcmp(".byte", inst->name) == 0) {
			if (parse_data(&now, &s_name, &value) == PARSE_DIGIT)
				line->y86bin.codes[0] = HPACK((value >> 4) & 0xF, (value >> 0) & 0xF);
			else {
				symbol_t *ps = find_symbol(s_name);
				if (ps != NULL) {
					value = ps->addr;
					line->y86bin.codes[0] = HPACK((value >> 4) & 0xF, (value >> 0) & 0xF);
				}
				else
					add_reloc(s_name, &line->y86bin);
			}
		}

		if (strcmp(".word", inst->name) == 0) {
			if (parse_data(&now, &s_name, &value) == PARSE_DIGIT) {
				line->y86bin.codes[0] = HPACK((value >> 4) & 0xF, (value >> 0) & 0xF);
				line->y86bin.codes[1] = HPACK((value >> 12) & 0xF, (value >> 8) & 0xF);
			}
			else {
				symbol_t *ps = find_symbol(s_name);
				if (ps != NULL) {
					value = ps->addr;
					line->y86bin.codes[0] = HPACK((value >> 4) & 0xF, (value >> 0) & 0xF);
					line->y86bin.codes[1] = HPACK((value >> 12) & 0xF, (value >> 8) & 0xF);
				}
				else
					add_reloc(s_name, &line->y86bin);
			}
			
		}

		if (strcmp(".long", inst->name) == 0) {
			if (parse_data(&now, &s_name, &value) == PARSE_DIGIT) {
				line->y86bin.codes[0] = HPACK((value >> 4) & 0xF, (value >> 0) & 0xF);
				line->y86bin.codes[1] = HPACK((value >> 12) & 0xF, (value >> 8) & 0xF);
				line->y86bin.codes[2] = HPACK((value >> 20) & 0xF, (value >> 16) & 0xF);
				line->y86bin.codes[3] = HPACK((value >> 28) & 0xF, (value >> 24) & 0xF);
			}
			else {
				symbol_t *ps = find_symbol(s_name);
				if (ps != NULL) {
					value = ps->addr;
					line->y86bin.codes[0] = HPACK((value >> 4) & 0xF, (value >> 0) & 0xF);
					line->y86bin.codes[1] = HPACK((value >> 12) & 0xF, (value >> 8) & 0xF);
					line->y86bin.codes[2] = HPACK((value >> 20) & 0xF, (value >> 16) & 0xF);
					line->y86bin.codes[3] = HPACK((value >> 28) & 0xF, (value >> 24) & 0xF);
				}
				else
					add_reloc(s_name, &line->y86bin);
			}
		}

		line->type = TYPE_INS;
		return line->type;
	}


	SKIP_BLANK(now)	
	if (IS_END(now)) {
		line->type = TYPE_INS;
		return line->type;
	}

	line->type = TYPE_ERR;
	return line->type;
}

/*
 * assemble: assemble an y86 file (e.g., 'asum.ys')
 * args
 *     in: point to input file (an y86 assembly file)
 *
 * return
 *     0: success, assmble the y86 file to a list of line_t
 *     -1: error, try to print err information (e.g., instr type and line number)
 */
int assemble(FILE *in) {
    static char asm_buf[MAX_INSLEN]; /* the current line of asm code */
    line_t *line;
    int slen;
    char *y86asm;

    /* read y86 code line-by-line, and parse them to generate raw y86 binary code list */
    while (fgets(asm_buf, MAX_INSLEN, in) != NULL) {
        slen = strlen(asm_buf);
        if ((asm_buf[slen-1] == '\n') || (asm_buf[slen-1] == '\r')) { 
            asm_buf[--slen] = '\0'; /* replace terminator */
        }

        /* store y86 assembly code */
        y86asm = (char *)malloc(sizeof(char) * (slen + 1)); // free in finit
        strcpy(y86asm, asm_buf);

        line = (line_t *)malloc(sizeof(line_t)); // free in finit
        memset(line, '\0', sizeof(line_t));

        /* set defualt */
        line->type = TYPE_COMM;
        line->y86asm = y86asm;
        line->next = NULL;

        /* add to y86 binary code list */
        y86bin_listtail->next = line;
        y86bin_listtail = line;
        y86asm_lineno ++;

        if (y86asm_lineno == 1)
        	y86bin_listhead->next = line;

        /* parse */
        if (parse_line(line) == TYPE_ERR)
            return -1;
    }

    /* skip line number information in err_print() */
    y86asm_lineno = -1;
    return 0;
}

/*
 * relocate: relocate the raw y86 binary code with symbol address
 *
 * return
 *     0: success
 *     -1: error, try to print err information (e.g., addr and symbol)
 */
int relocate(void) {
    reloc_t *rtmp = NULL;
    rtmp = reltab->next;
    while (rtmp != NULL) {
        /* find symbol */
        symbol_t *ps = find_symbol(rtmp->name);
    	int id = rtmp->y86bin->bytes;
        /* relocate y86bin according itype */
        if ((ps != NULL) && (id == 5)) {
    		rtmp->y86bin->codes[1] = HPACK(((ps->addr >> 4) & 0xF), (ps->addr >> 0) & 0xF);
    		rtmp->y86bin->codes[2] = HPACK(((ps->addr >> 12) & 0xF), (ps->addr >> 8) & 0xF);
    		rtmp->y86bin->codes[3] = HPACK(((ps->addr >> 20) & 0xF), (ps->addr >> 16) & 0xF);
    		rtmp->y86bin->codes[4] = HPACK(((ps->addr >> 28) & 0xF), (ps->addr >> 24) & 0xF);
    	}
    	else if ((ps != NULL) && (id == 6)) {
    		rtmp->y86bin->codes[2] = HPACK(((ps->addr >> 4) & 0xF), (ps->addr >> 0) & 0xF);
    		rtmp->y86bin->codes[3] = HPACK(((ps->addr >> 12) & 0xF), (ps->addr >> 8) & 0xF);
    		rtmp->y86bin->codes[4] = HPACK(((ps->addr >> 20) & 0xF), (ps->addr >> 16) & 0xF);
    		rtmp->y86bin->codes[5] = HPACK(((ps->addr >> 28) & 0xF), (ps->addr >> 24) & 0xF);
    	}
    	else if ((ps != NULL) && (id == 4)) {//.long
    		rtmp->y86bin->codes[0] = HPACK(((ps->addr >> 4) & 0xF), (ps->addr >> 0) & 0xF);
    		rtmp->y86bin->codes[1] = HPACK(((ps->addr >> 12) & 0xF), (ps->addr >> 8) & 0xF);
    		rtmp->y86bin->codes[2] = HPACK(((ps->addr >> 20) & 0xF), (ps->addr >> 16) & 0xF);
    		rtmp->y86bin->codes[3] = HPACK(((ps->addr >> 28) & 0xF), (ps->addr >> 24) & 0xF);
    	}
    	else if ((ps != NULL) && (id == 2)) {//.word
    		rtmp->y86bin->codes[0] = HPACK(((ps->addr >> 4) & 0xF), (ps->addr >> 0) & 0xF);
    		rtmp->y86bin->codes[1] = HPACK(((ps->addr >> 12) & 0xF), (ps->addr >> 8) & 0xF);
    	}
    	else if ((ps != NULL) && (id == 1)) {//.byte
    		rtmp->y86bin->codes[0] = HPACK(((ps->addr >> 4) & 0xF), (ps->addr >> 0) & 0xF);
    	}
    	else if (ps != NULL) {}

    	else {
    		err_print("Unknown symbol:'%s'", rtmp->name);
    		return -1;
    	}
        /* next */
        rtmp = rtmp->next;
    }
    return 0;
}

/*
 * binfile: generate the y86 binary file
 * args
 *     out: point to output file (an y86 binary file)
 *
 * return
 *     0: success
 *     -1: error
 */
int binfile(FILE *out) {
	int writepos = 0;
    /* prepare image with y86 binary code */
	line_t *write = NULL;
	write = y86bin_listhead->next;
    /* binary write y86 code to output file (NOTE: see fwrite()) */
    while (write != NULL) {
    	if (write->y86bin.addr == writepos) {
    		fwrite(write->y86bin.codes, 1, write->y86bin.bytes, out);
    		writepos += write->y86bin.bytes;
    	}
    	else if ((write->y86bin.addr > writepos) && (write->y86bin.bytes > 0)) {
    		char *zero = (char*)malloc(sizeof(char));
    		*zero = '\0';
    		int i;
    		for (i = 1; i <= write->y86bin.addr - writepos; ++i)
    			fwrite(zero, 1, 1, out);
    		writepos += write->y86bin.addr - writepos;
    		fwrite(write->y86bin.codes, 1, write->y86bin.bytes, out);
    		writepos += write->y86bin.bytes;
    		free(zero);
    	}
    	else {}
    	write = write->next;
    }
    return 0;
}


/* whether print the readable output to screen or not ? */
bool_t screen = FALSE; 

static void hexstuff(char *dest, int value, int len) {
    int i;
    for (i = 0; i < len; i++) {
        char c;
        int h = (value >> 4*i) & 0xF;
        c = h < 10 ? h + '0' : h - 10 + 'a';
        dest[len-i-1] = c;
    }
}

void print_line(line_t *line) {
    char buf[26];

    /* line format: 0xHHH: cccccccccccc | <line> */
    if (line->type == TYPE_INS) {
        bin_t *y86bin = &line->y86bin;
        int i;
        
        strcpy(buf, "  0x000:              | ");
        
        hexstuff(buf+4, y86bin->addr, 3);
        if (y86bin->bytes > 0)
            for (i = 0; i < y86bin->bytes; i++)
                hexstuff(buf+9+2*i, y86bin->codes[i]&0xFF, 2);
    } else {
        strcpy(buf, "                      | ");
    }

    printf("%s%s\n", buf, line->y86asm);
}

/* 
 * print_screen: dump readable binary and assembly code to screen
 * (e.g., Figure 4.8 in ICS book)
 */
void print_screen(void) {
    line_t *tmp = y86bin_listhead->next;
    
    /* line by line */
    while (tmp != NULL) {
        print_line(tmp);
        tmp = tmp->next;
    }
}

/* init and finit */
void init(void) {
    reltab = (reloc_t *)malloc(sizeof(reloc_t)); // free in finit
    memset(reltab, 0, sizeof(reloc_t));

    symtab = (symbol_t *)malloc(sizeof(symbol_t)); // free in finit
    memset(symtab, 0, sizeof(symbol_t));

    y86bin_listhead = (line_t *)malloc(sizeof(line_t)); // free in finit
    memset(y86bin_listhead, 0, sizeof(line_t));
    y86bin_listtail = y86bin_listhead;
    y86asm_lineno = 0;
}

void finit(void) {
    reloc_t *rtmp = NULL;
    do {
        rtmp = reltab->next;
        if (reltab->name) 
            free(reltab->name);
        free(reltab);
        reltab = rtmp;
    } while (reltab);
    
    symbol_t *stmp = NULL;
    do {
        stmp = symtab->next;
        if (symtab->name) 
            free(symtab->name);
        free(symtab);
        symtab = stmp;
    } while (symtab);

    line_t *ltmp = NULL;
    do {
        ltmp = y86bin_listhead->next;
        if (y86bin_listhead->y86asm) 
            free(y86bin_listhead->y86asm);
        free(y86bin_listhead);
        y86bin_listhead = ltmp;
    } while (y86bin_listhead);
}

static void usage(char *pname) {
    printf("Usage: %s [-v] file.ys\n", pname);
    printf("   -v print the readable output to screen\n");
    exit(0);
}

int main(int argc, char *argv[]) {
    int rootlen;
    char infname[512];
    char outfname[512];
    int nextarg = 1;
    FILE *in = NULL, *out = NULL;
    
    if (argc < 2)
        usage(argv[0]);
    
    if (argv[nextarg][0] == '-') {
        char flag = argv[nextarg][1];
        switch (flag) {
          case 'v':
            screen = TRUE;
            nextarg++;
            break;
          default:
            usage(argv[0]);
        }
    }

    /* parse input file name */
    rootlen = strlen(argv[nextarg])-3;
    /* only support the .ys file */
    if (strcmp(argv[nextarg]+rootlen, ".ys"))
        usage(argv[0]);
    
    if (rootlen > 500) {
        err_print("File name too long");
        exit(1);
    }


    /* init */
    init();

    
    /* assemble .ys file */
    strncpy(infname, argv[nextarg], rootlen);
    strcpy(infname+rootlen, ".ys");
    in = fopen(infname, "r");
    if (!in) {
        err_print("Can't open input file '%s'", infname);
        exit(1);
    }
    
    if (assemble(in) < 0) {
        err_print("Assemble y86 code error");
        fclose(in);
        exit(1);
    }
    fclose(in);


    /* relocate binary code */
    if (relocate() < 0) {
        err_print("Relocate binary code error");
        exit(1);
    }


    /* generate .bin file */
    strncpy(outfname, argv[nextarg], rootlen);
    strcpy(outfname+rootlen, ".bin");
    out = fopen(outfname, "wb");
    if (!out) {
        err_print("Can't open output file '%s'", outfname);
        exit(1);
    }

    if (binfile(out) < 0) {
        err_print("Generate binary file error");
        fclose(out);
        exit(1);
    }
    fclose(out);
    
    /* print to screen (.yo file) */
    if (screen)
       print_screen(); 

    /* finit */
    finit();
    return 0;
}



