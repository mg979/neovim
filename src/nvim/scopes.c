#include "garray.h"
#include "maphash.h"
#include "scopes.h"

int ID = 0; // incremented in add_context()
int KM_ID = 0; // incremented in add_context_mappings()
const int GROW_SIZE = 10;

garray_T contexts = GA_EMPTY_INIT_VALUE;

#define CONTEXTS ((MapsContext *)contexts.ga_data)

static MapsContext *allocate_empty(void);
static int add_context(char *name, MapsContextScope scope);
static void free_context(MapsContext *cxt);

static bool initialized = false;


/******************************************************************************
 * Public functions
 *****************************************************************************/

/// Initialize default contexts.
void contexts_initialize(void)
{
  if (initialized)
    return;
  contexts = (garray_T)GA_INIT(sizeof(MapsContext *), GROW_SIZE);
  add_context("global", SCOPE_GLOBAL);
  add_context("current_buffer", SCOPE_BUFFER);
  add_context("current_window", SCOPE_WINDOW);

  initialized = true;
}

/// Update current buffer scope.
void contexts_update_current_buf(void)
{
  (CONTEXTS + SCOPE_BUFFER)->mappings[0] = curbuf->b_maphash[0];
}


/// Free all contexts with their allocated resources.
void contexts_free(void)
{
  MapsContext *cur = CONTEXTS;
  for (int i = 0; i < contexts.ga_len; i++) {
    free_context(cur += i);
  }
}

/// Get a scoped context by its id, that corresponds to the position in ga->ga_data.
MapsContext *contexts_with_id(int id)
{
  return CONTEXTS + id;
}



/******************************************************************************
 * nvim API
 *****************************************************************************/

/// Create an user context. Return the new context's id, or -1 if the context
/// could not be created.
int nvim_context_create(char *name, char *scope_str)
{
  // check if a context with the same name exists
  for (int i = 0; i < contexts.ga_len; i++) {
    if (strcmp((CONTEXTS + i)->name, name) == 0) {
      return -1;
    }
  }
  // check the scope string argument
  MapsContextScope scope = SCOPE_GLOBAL;
  if (strcmp(scope_str, "buffer") == 0) {
    scope = SCOPE_BUFFER;
  } else if (strcmp(scope_str, "window") == 0) {
    scope = SCOPE_WINDOW;
  } else if (strcmp(scope_str, "global") != 0) {
    return -1;
  }
  return add_context(name, scope);
}

/// Define a set of mappings, to be used by an user context.
/// Returns the id of the new set, or -1 if unsuccessful.
int nvim_context_define_mappings(MapsContextKeymaps *keymaps)
{
  return KM_ID;
}

/// Return the id of a context with a given name, or -1 if no such context
/// exists.
int nvim_context_get_id(char *name)
{
  for (int i = 0; i < contexts.ga_len; i++) {
    if (strcmp((CONTEXTS + i)->name, name) == 0) {
      return (CONTEXTS + i)->id;
    }
  }
  return -1;
}

/// Set the enabled state for CONTEXTS[id]. Return success.
bool nvim_context_enable(int id, bool enabled)
{
  if (id < 0 || id >= contexts.ga_len) {
    return false;
  }
  (CONTEXTS + id)->enabled = enabled;
  return true;
}



/******************************************************************************
 * Static functions
 *****************************************************************************/

/// Allocate a new empty scoped context and return it.
static MapsContext *allocate_empty()
{
  MapsContext *new = malloc(sizeof(MapsContext));
  new->enabled = true;
  new->name = NULL;
  new->id = 0;
  new->scope = SCOPE_GLOBAL;
  new->priority = SCOPE_PRIORITY_GLOBAL;
  memset(new->mappings, 0, sizeof(new->mappings));
  return new;
}

/// Add a new context to the global list. Return context id.
static int add_context(char *name, MapsContextScope scope)
{
  MapsContext *new = allocate_empty();

  new->scope = scope;

  // allocate and set context name
  char *scope_name = malloc(sizeof(char) * strlen(name) + 1);
  strcpy(scope_name, name);
  new->name = scope_name;

  // set id, will be the position in CONTEXTS[]
  new->id = ID++;

  GA_APPEND(MapsContext *, &contexts, new);
  return ID - 1;
}

/// Free a context and its allocated resources.
static void free_context(MapsContext *cxt)
{
  if (cxt != NULL) {
    xfree(cxt->name);
    mapblock_T **mpp;
    for (int hash = 0; hash < MAX_MAPHASH; ++hash) {
      mpp = &cxt->mappings[hash];
      while (mapblock_free(mpp) != NULL) {}
    }
    free(cxt->mappings);
    free(cxt);
  }
  // make NULL also CONTEXTS[cxt->id], since the context is invalid now
  // cxt->id MUST correspond to the position in the scopes array!
  ((MapsContext **)contexts.ga_data)[cxt->id] = NULL;
}

