#ifndef NVIM_SCOPES_H
#define NVIM_SCOPES_H

#include "garray.h"
#include "nvim/buffer_defs.h"
#include "nvim/api/private/defs.h"
#include "nvim/api/private/helpers.h"

typedef struct MapsContextKeymaps {
  int mode;
  char *lhs;
  char *rhs;
  Dict(keymap) *opts;
} MapsContextKeymaps;

/// The scope of contexts can be (in reversed order of precedence):
/// - global
/// - buffer-local
/// - window-local
///
/// Their processing order can be affected by:
/// - context.id: scopes defined later have higher precedence
/// - context.priority: defaults to context scope, but can be different
typedef enum {
  SCOPE_GLOBAL = 0,
  SCOPE_BUFFER = 1,
  SCOPE_WINDOW = 2,
} MapsContextScope;

/// By default, contexts have the priority that is appropriate for their scope.
/// User contexts can have a different priority, if they want to override other
/// default/user contexts beyond what their scope would allow.
/// Eg. a context with buffer scope, but max priority, will have higher
/// precedence than window-local contexts.
typedef enum {
  SCOPE_PRIORITY_GLOBAL = 0,
  SCOPE_PRIORITY_BUFFER = 1,
  SCOPE_PRIORITY_WINDOW = 2,
  SCOPE_PRIORITY_MAX = 3,
} MapsContextPriority;

/// Structure for scoped contexts
typedef struct scope {
  bool enabled;
  char *name;
  int id; // incremental id, the position in scopes[]
  MapsContextScope scope; // Either global, buffer or window.
  MapsContextPriority priority; // By default, priority is the same as the declared scope.
  mapblock_T *mappings[MAX_MAPHASH]; // List of mappings associated to the context.
} MapsContext;

/// Array with currently defined scoped contexts.
///
/// As a bare minimum, there are always 3 valid contexts:
/// - global context (index 0)
/// - current buffer context (index 1)
/// - current window context (index 2)
///
/// Additional contexts can be defined by users.
extern garray_T contexts;

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "scopes.h.generated.h"
#endif
#endif // NVIM_SCOPES_H
