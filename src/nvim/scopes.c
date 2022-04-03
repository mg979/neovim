#include "garray.h"
#include "maphash.h"
#include "message.h"
#include "scopes.h"

int ID = 0; // incremented in add_context()
const int GROW_SIZE = 10;

/// Growing array of pointers to MapsContext structs.
garray_T contexts;

static int add_context(char *name, MapsContextScope scope);
static void free_context(MapsContext *cxt);

static bool initialized = false;

#define CONTEXTS(id) (((MapsContext **)contexts.ga_data)[(id)])


/******************************************************************************
 * Public functions
 *****************************************************************************/

/// Initialize default contexts.
void contexts_initialize(void)
{
  if (initialized)
    return;
  ga_init(&contexts, sizeof(MapsContext *), GROW_SIZE);
  add_context("global", SCOPE_GLOBAL);
  add_context("current_buffer", SCOPE_BUFFER);
  add_context("current_window", SCOPE_WINDOW);

  contexts_update_current_buf();
  initialized = true;
}

/// Update current buffer scope.
void contexts_update_current_buf(void)
{
  CONTEXTS(SCOPE_BUFFER)->mappings = curbuf->b_maphash;
}


/// Free all contexts with their allocated resources.
void contexts_free(void)
{
  MapsContext *cur = CONTEXTS(SCOPE_GLOBAL);
  for (int i = 0; i < contexts.ga_len; i++) {
    free_context(cur += i);
  }
}

/// Get a context by its id, that corresponds to the position in ga->ga_data.
MapsContext *contexts_get(int id)
{
  return CONTEXTS(id);
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
    if (strcmp(CONTEXTS(i)->name, name) == 0) {
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
/// Returns success.
bool nvim_context_define_mappings(int id, MapsContextKeymaps *keymaps)
{
  if (id <= SCOPE_WINDOW) {
    emsg(_("Not possible to redefine mappings for default contexts."));
    return false;
  }
  return true;
}

/// Return the id of a context with a given name, or -1 if no such context
/// exists.
int nvim_context_get_id(char *name)
{
  for (int i = 0; i < contexts.ga_len; i++) {
    if (strcmp(CONTEXTS(i)->name, name) == 0) {
      return CONTEXTS(i)->id;
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
  CONTEXTS(id)->enabled = enabled;
  return true;
}



/******************************************************************************
 * Static functions
 *****************************************************************************/

/// Add a new context to the global list. Return context id.
static int add_context(char *name, MapsContextScope scope)
{
  MapsContext *new = xcalloc(1, sizeof(MapsContext));

  new->enabled = true;
  new->scope = scope;

  // allocate and set context name
  char *scope_name = xmalloc(sizeof(char) * strlen(name) + 1);
  strcpy(scope_name, name);
  new->name = scope_name;

  // point to global/buffer maphash if default contexts
  switch (scope) {
  case SCOPE_GLOBAL:
    new->mappings = maphash;
    break;
  case SCOPE_BUFFER:
    new->mappings = curbuf->b_maphash;
    break;
  case SCOPE_WINDOW:
    // not implemented yet
    break;
  default:
    // stays NULL, must be bound after creating mappings
    break;
  }

  // set id, will be the position in CONTEXTS[]
  new->id = ID;

  GA_APPEND(MapsContext *, &contexts, new);
  return ID++;
}

/// Free a context and its allocated resources.
static void free_context(MapsContext *cxt)
{
  if (cxt != NULL) {
    xfree(cxt->name);
    mapblock_T **mpp;
    if (cxt->id > SCOPE_WINDOW) {
      for (int hash = 0; hash < MAX_MAPHASH; ++hash) {
        mpp = &cxt->mappings[hash];
        while (mapblock_free(mpp) != NULL) {}
      }
      free(cxt->mappings);
    }
    free(cxt);
  }
  // make NULL also CONTEXTS[cxt->id], since the context is invalid now
  // cxt->id MUST correspond to the position in the scopes array!
  CONTEXTS(cxt->id) = NULL;
}

