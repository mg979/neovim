#ifndef NVIM_GETCHAR_H
#define NVIM_GETCHAR_H

#include "nvim/buffer_defs.h"
#include "nvim/ex_cmds_defs.h"
#include "nvim/maphash.h"
#include "nvim/os/fileio.h"
#include "nvim/types.h"
#include "nvim/vim.h"

// Argument for flush_buffers().
typedef enum {
  FLUSH_MINIMAL,
  FLUSH_TYPEAHEAD,  // flush current typebuf contents
  FLUSH_INPUT,  // flush typebuf and inchar() input
} flush_buffers_T;

/// Maximum number of streams to read script from
enum { NSCRIPT = 15, };

/// Streams to read script from
extern FileDescriptor *scriptin[NSCRIPT];

#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "getchar.h.generated.h"
#endif
#endif  // NVIM_GETCHAR_H
