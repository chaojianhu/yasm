/*
 * YASM module loader
 *
 *  Copyright (C) 2002  Peter Johnson
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#define YASM_LIB_INTERNAL
#include <libyasm.h>
/*@unused@*/ RCSID("$IdPath$");

#include "ltdl.h"

#include "yasm-module.h"


typedef struct module {
    SLIST_ENTRY(module) link;
    /*@only@*/ char *type;	    /* module type */
    /*@only@*/ char *keyword;	    /* module keyword */
    lt_dlhandle handle;		    /* dlopen handle */
} module;

SLIST_HEAD(modulehead, module) modules = SLIST_HEAD_INITIALIZER(modules);

/* NULL-terminated list of all possibly available object format keywords.
 * Could improve this a little by generating automatically at build-time.
 */
/*@-nullassign@*/
const char *dbgfmts[] = {
    "null",
    NULL
};
/*@=nullassign@*/

/* NULL-terminated list of all possibly available object format keywords.
 * Could improve this a little by generating automatically at build-time.
 */
/*@-nullassign@*/
const char *objfmts[] = {
    "dbg",
    "bin",
    "coff",
    NULL
};
/*@=nullassign@*/

/* NULL-terminated list of all possibly available parser keywords.
 * Could improve this a little by generating automatically at build-time.
 */
/*@-nullassign@*/
const char *parsers[] = {
    "nasm",
    NULL
};
/*@=nullassign@*/

/* NULL-terminated list of all possibly available preproc keywords.
 * Could improve this a little by generating automatically at build-time.
 */
/*@-nullassign@*/
const char *preprocs[] = {
    "nasm",
    "raw",
    NULL
};
/*@=nullassign@*/

static /*@dependent@*/ /*@null@*/ module *
load_module(const char *type, const char *keyword)
{
    module *m;
    char *name;
    lt_dlhandle handle;
    size_t typelen;

    /* See if the module has already been loaded. */
    SLIST_FOREACH(m, &modules, link) {
	if (yasm__strcasecmp(m->type, type) == 0 &&
	    yasm__strcasecmp(m->keyword, keyword) == 0)
	    return m;
    }

    /* Look for dynamic module.  First build full module name from keyword. */
    typelen = strlen(type);
    name = yasm_xmalloc(typelen+strlen(keyword)+2);
    strcpy(name, type);
    strcat(name, "_");
    strcat(name, keyword);
    handle = lt_dlopenext(name);

    if (!handle) {
	yasm_xfree(name);
	return NULL;
    }

    m = yasm_xmalloc(sizeof(module));
    m->type = name;
    name[typelen] = '\0';
    m->keyword = &name[typelen+1];
    m->handle = handle;
    SLIST_INSERT_HEAD(&modules, m, link);
    return m;
}

void
unload_modules(void)
{
    module *m;

    while (!SLIST_EMPTY(&modules)) {
	m = SLIST_FIRST(&modules);
	SLIST_REMOVE_HEAD(&modules, link);
	yasm_xfree(m->type);
	lt_dlclose(m->handle);
	yasm_xfree(m);
    }
}

void *
get_module_data(const char *type, const char *keyword, const char *symbol)
{
    char *name;
    /*@dependent@*/ module *m;
    void *data;

    /* Load module */
    m = load_module(type, keyword);
    if (!m)
	return NULL;

    name = yasm_xmalloc(strlen(keyword)+strlen(symbol)+11);

    strcpy(name, "yasm_");
    strcat(name, keyword);
    strcat(name, "_LTX_");
    strcat(name, symbol);

    /* Find and return data pointer: NULL if it doesn't exist */
    data = lt_dlsym(m->handle, name);

    yasm_xfree(name);
    return data;
}

void
list_objfmts(void (*printfunc) (const char *name, const char *keyword))
{
    int i;
    yasm_objfmt *of;

    /* Go through available list, and try to load each one */
    for (i = 0; objfmts[i]; i++) {
	of = load_objfmt(objfmts[i]);
	if (of)
	    printfunc(of->name, of->keyword);
    }
}

void
list_parsers(void (*printfunc) (const char *name, const char *keyword))
{
    int i;
    yasm_parser *p;

    /* Go through available list, and try to load each one */
    for (i = 0; parsers[i]; i++) {
	p = load_parser(parsers[i]);
	if (p)
	    printfunc(p->name, p->keyword);
    }
}

void
list_preprocs(void (*printfunc) (const char *name, const char *keyword))
{
    int i;
    yasm_preproc *p;

    /* Go through available list, and try to load each one */
    for (i = 0; preprocs[i]; i++) {
	p = load_preproc(preprocs[i]);
	if (p)
	    printfunc(p->name, p->keyword);
    }
}

void
list_dbgfmts(void (*printfunc) (const char *name, const char *keyword))
{
    int i;
    yasm_dbgfmt *p;

    /* Go through available list, and try to load each one */
    for (i = 0; dbgfmts[i]; i++) {
	p = load_dbgfmt(dbgfmts[i]);
	if (p)
	    printfunc(p->name, p->keyword);
    }
}
