#ifndef NVIM_MULTICURSOR_H
#define NVIM_MULTICURSOR_H

#include "nvim/vim.h"

#define CHECK_LIST                                                             \
  if (Cursors->list == NULL)                                                   \
  return reset()

/// Returns true if x comes before y
#define COMES_BEFORE(x, y)                                                     \
  ((x)->l < (y)->l || ((x)->l == (y)->l && (x)->a < (y)->a))

// Reminder: for offsets, varnumber_T must be used as type

/// Maximal number of regions/cursors
enum { MAX_REGIONS = INT_MAX };

// forward declarations
typedef struct _region_T region_T;  // region structure
struct _multicursors;               // global structure
struct _mc_pattern;                 // node for patterns list
struct _mc_vt_generic;              // table for generic methods
struct _mc_vt_commands;             // table for commands
struct _mc_vt_region;               // table for region methods
struct _mc_vt_search;               // table for search methods

/*
 * This is the global structure that is exposed to the rest of the program:
 * there is a single instance of this structure, called _Cursors, globally
 * accessed through its pointer *Cursors
 *
 * For multiple cursors to be active, so that their mappings and related
 * functionalities will work, there must be at least one region in the list,
 * therefore one must check (Cursors->list != NULL)
 *
 * One could also check (Cursors->len) but better not, even if it shouldn't
 * happen that this variable is non-zero when there are no regions
 */
EXTERN struct _multicursors *Cursors;

struct _multicursors {
  // members
  region_T *list;      // pointer to the last added region
  int n;               // number of regions present in the list
  int last_id;         // id of last added region (incremental)
  int index;           // index of most recently (currently) selected region
  bool SelectionMode;  // true for selection-mode, false for cursor-mode

  char *pattern;                 // current joined search pattern
  struct _mc_pattern *patterns;  // search patterns

  // basic methods
  int (*len)(void);          // return the number of regions, but safely
  region_T *(*reset)(void);  // reset multicursors state

  // methods tables
  struct _mc_vt_generic *Functions;
  struct _mc_vt_commands *Commands;
  struct _mc_vt_region *Region;
  struct _mc_vt_search *Search;
};

/*
 * Node for patterns list. All patterns are then joined and will be written to
 * Cursors->pattern
 */
struct _mc_pattern {
  char *pattern;
  bool word;  // use word boundaries
  struct _mc_pattern *next;
};

// definitions for methods tables

struct _mc_vt_generic {
  void (*update)(void);
  region_T *(*reorder)(void);
  region_T *(*new_from_yank)(void);
  region_T *(*new_from_search)(void);
  region_T *(*new_cursor)(linenr_T l, colnr_T a);
  region_T *(*new_selection)(linenr_T l, colnr_T a, linenr_T L, colnr_T b);
};

struct _mc_vt_commands {
  region_T *(*cursor_at_pos)(void);
  region_T *(*find_under)(void);
  region_T *(*get_next)(bool forward);
  region_T *(*skip_and_get_next)(bool forward);
};

struct _mc_vt_region {
  void (*motion)(region_T *);
  void (*get_text)(region_T *);
};

struct _mc_vt_search {
  void (*add)(const char *);
  void (*toggle_word)(void);
  void (*clear)(void);
  void (*join)(void);
};

/*
 * There is a single type for selections/cursors; when in cursor-mode, the
 * fields for the right edge and the orientation will be ignored
 */
struct _region_T {
  linenr_T l;      // line number                              (left)
  colnr_T a;       // column number                            (left)
  colnr_T va;      // offset when in virtualedit               (left)
  colnr_T wa;      // requested column for vertical movement   (left)
  linenr_T L;      // line number                              (right)
  colnr_T b;       // column number                            (right)
  colnr_T vb;      // offset when in virtualedit               (right)
  colnr_T wb;      // requested column for vertical movement   (right)
  bool inverted;   // orientation (to determine the anchor)
  int id;          // incremental id
  region_T *next;  // next region in the list
};

#endif  // NVIM_MULTICURSOR_H
