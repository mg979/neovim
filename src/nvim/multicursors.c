// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

//
// multicursors.c:   routines to execute commands in normal/ex/insert modes at
//                   multiple positions.
//

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "nvim/multicursors.h"
#include "nvim/vim.h"

/******************************************************************************
 * static variables
 *****************************************************************************/

static bool NoAutoReorder = false;

/******************************************************************************
 * Prototypes for methods
 *****************************************************************************/

static void update(void);
static region_T *reorder(void);
static region_T *new_from_yank(void);
static region_T *new_from_search(void);
static region_T *new_cursor(linenr_T l, colnr_T a);
static region_T *new_selection(linenr_T l, colnr_T a, linenr_T L, colnr_T b);

static region_T *add_cursor_at_pos(void);
static region_T *find_under(void);
static region_T *get_next(bool forward);
static region_T *skip_and_get_next(bool forward);

static void motion(region_T *r);
static void get_text(region_T *r);

static void add_search(const char *);
static void toggle_word(void);
static void clear_search(void);
static void join_search(void);

static int len(void);
static region_T *reset(void);

/******************************************************************************
 * Methods tables
 *****************************************************************************/

static struct _mc_vt_generic mcGeneric = {
    update, reorder, new_from_yank, new_from_search, new_cursor, new_selection,
};

static struct _mc_vt_commands mcCommands
    = {add_cursor_at_pos, find_under, get_next, skip_and_get_next};

static struct _mc_vt_region mcRegion = {
    motion,
    get_text,
};

static struct _mc_vt_search mcSearch = {
    add_search,
    toggle_word,
    clear_search,
    join_search,
};

/******************************************************************************
 * Globally accessible structure
 *
 * Methods in this structure are organized in different tables, so that they
 * are invoked like this (examples):
 *
 *    Cursors->Functions->update()
 *    Cursors->Commands->add_cursor_at_pos()
 *    Cursors->Region->get_text(&r)
 *
 *****************************************************************************/

static struct _multicursors _Cursors
    = {NULL, 0,     0,          0,           false,     NULL,     NULL,
       len,  reset, &mcGeneric, &mcCommands, &mcRegion, &mcSearch};

struct _multicursors *Cursors = &_Cursors;

static region_T *_init_region(linenr_T l, linenr_T L, colnr_T b, colnr_T a);

/******************************************************************************
 * Generic methods implementations
 *
 * These methods are generally invoked internally, but are also available to
 * plugins.
 *****************************************************************************/

static void update()
{
}

static region_T *reorder()
{
  CHECK_LIST;
  region_T *h = Cursors->list;  // the head of the list inside the loop
  region_T *n = h->next;        // the next element

  for (; n; n = h->next) {
    if (COMES_BEFORE(h, n)) {
      // we must find the index where there aren't regions that come after it
      // so we go to the top of the list and find the first good spot
      // first we check if the region should go on top
      if (COMES_BEFORE(Cursors->list, n)) {
        h->next = n->next;        // n will be displaced
        n->next = Cursors->list;  // and goes before current top of the list
        Cursors->list = n;        // new top of the list becomes n
      } else {
        // iterate the rest of the list until we find a suitable spot
        // FIXME: not sure if it works when the region will be the tail
        for (region_T *p = Cursors->list; p->next; p = p->next) {
          if (COMES_BEFORE(p->next, n)) {
            h->next = n->next;  // n will change place, h must go on
            n->next = p->next;
            p->next = n;
            break;
          }
        }
      }
    } else {
      h = n;  // advance
    }
  }
  return Cursors->list;
}

static region_T *new_cursor(linenr_T l, colnr_T a)
{
  region_T *r = _init_region(l, l, a, a);
  return r;
}

static region_T *new_selection(linenr_T l, colnr_T a, linenr_T L, colnr_T b)
{
  region_T *r = _init_region(l, L, a, b);
  return r;
}

static region_T *new_from_yank()
{
  region_T *r = _init_region(0, 0, 0, 0);
  return r;
}

static region_T *new_from_search()
{
  region_T *r = _init_region(0, 0, 0, 0);
  return r;
}

/******************************************************************************
 * Commands implementations
 *
 * These methods are supposed to be bound to mappings, and executed by the
 * user. Generally they don't take parameters, if they do, it could mean that
 * they are variants of the same command, bound to different mappings.
 *****************************************************************************/

static region_T *add_cursor_at_pos()
{
  region_T *r = _init_region(0, 0, 0, 0);
  return r;
}

static region_T *find_under()
{
  region_T *r = _init_region(0, 0, 0, 0);
  return r;
}

static region_T *get_next(bool forward)
{
  region_T *r = _init_region(0, 0, 0, 0);
  return r;
}

static region_T *skip_and_get_next(bool forward)
{
  region_T *r = _init_region(0, 0, 0, 0);
  return r;
}

/******************************************************************************
 * Region methods implementations
 *
 * These methods act on the single regions, for the purpose of altering them,
 * getting their text, etc. They always need a region as first parameter.
 *****************************************************************************/

static void motion(region_T *r)
{
}

static void get_text(region_T *r)
{
}

/******************************************************************************
 * Search methods implementations
 *
 * These methods are used to add/edit/remove search patterns.
 *****************************************************************************/

static void add_search(const char *pattern)
{
  struct _mc_pattern *new = xcalloc(sizeof(struct _mc_pattern), 0);
  if (Cursors->patterns == NULL)
    Cursors->patterns = new;
  else
    Cursors->patterns->next = new;
  join_search();
}

/*
 * Toggle the use of word boundaries for the last added pattern.
 */
static void toggle_word()
{
  Cursors->patterns->word = !Cursors->patterns->word;
}

/*
 * Free all registered patterns.
 */
static void clear_search()
{
  xfree(Cursors->pattern);
  struct _mc_pattern *pn, *p = Cursors->patterns;
  while (p != NULL) {
    pn = p->next;
    xfree(p);
    p = pn;
  }
}

/**
 * Join all search patterns in a single string. Force 'very nomagic'.
 */
static void join_search()
{
  xfree(Cursors->pattern);  // free previous pattern
  size_t slen = 2;          // because of "\V"

  // find new pattern length
  struct _mc_pattern *p = Cursors->patterns;
  for (; p != NULL; p = p->next) {
    slen += strlen(p->pattern);
    if (p->word)  // word boundaries
      slen += 4;
    if (p->next)  // will add "\|"
      slen += 2;
  }

  // join current patterns and assign the result to Cursors->pattern
  char *pat = xmalloc(sizeof(char) * slen + 1);
  strcpy(pat, "\\V");
  for (p = Cursors->patterns; p != NULL; p = p->next) {
    if (p->word) {
      strcat(pat, "\\<");
      strcat(pat, p->pattern);
      strcat(pat, "\\>");
    } else {
      strcat(pat, p->pattern);
    }
    if (p->next)
      strcat(pat, "\\|");
  }
  Cursors->pattern = pat;
}

/******************************************************************************
 * Generic helpers implementations
 *****************************************************************************/

/*
 * Initialize a region with given positions.
 * If the region is located before other regions in the buffer, find its
 * correct position in the list, so that all regions keep being consecutive.
 *
 * @param l   line number of left edge
 * @param L   line number of right edge
 * @param a   column number of left edge
 * @param b   column number of right edge
 *
 * @return    Cursors->list
 */

static region_T *_init_region(linenr_T l, linenr_T L, colnr_T b, colnr_T a)
{
  region_T *r = xcalloc(sizeof(region_T), 0);
  ++Cursors->n;                // increment regions counter
  r->id = ++Cursors->last_id;  // assign new id (and increment global id)
  r->l = l;
  r->L = L;
  r->a = a;
  r->wa = a;
  r->b = b;
  r->wb = b;

  // in these cases we just append the new region to the list
  if (NoAutoReorder || !Cursors->list || COMES_BEFORE(r, Cursors->list)) {
    r->next = Cursors->list;  // append the new region to the list
    Cursors->list = r;        // update list head
    return r;
  }

  // find the right insertion point for the region that must be reordered
  region_T **pp = &Cursors->list->next;
  for (; *pp; pp = &(*pp)->next) {
    if (COMES_BEFORE(*pp, r)) {
      r->next = *pp;
      *pp = r;
      return r;
    }
  }

  // not added yet.. then it's the tail
  return *pp = r;
}

/*
 * Return the number of regions, but if this is erroneously non-zero (while the
 * list is NULL), reset multicursors and return 0.
 */
int len()
{
  if (Cursors->list == NULL) {
    if (Cursors->n)
      reset();
    return 0;
  }
  return Cursors->n;
}

static region_T *reset()
{
  while (Cursors->list != NULL) {
    region_T *r = Cursors->list;
    Cursors->list = r->next;
    xfree(r);
  }
  clear_search();
  Cursors->n = 0;
  Cursors->last_id = 0;  // this variable cannot be decremented, only reset
  Cursors->SelectionMode = 0;
  return NULL;
}
