#ifndef NVIM_MAPHASH_H
#define NVIM_MAPHASH_H

#include "nvim/buffer_defs.h"
#include "nvim/ex_cmds_defs.h"
#include "nvim/os/fileio.h"
#include "nvim/types.h"
#include "nvim/vim.h"

// Table used for global mappings.
extern mapblock_T *(maphash[MAX_MAPHASH]);

/******************************************************************************
 * Map arguments
 *****************************************************************************/

/// Values for "noremap" argument of ins_typebuf()
///
/// Also used for map->m_noremap and menu->noremap[].
enum RemapValues {
  REMAP_YES = 0,  ///< Allow remapping.
  REMAP_NONE = -1,  ///< No remapping.
  REMAP_SCRIPT = -2,  ///< Remap script-local mappings only.
  REMAP_SKIP = -3,  ///< No remapping for first char.
};

/// All possible |:map-arguments| usable in a |:map| command.
///
/// The <special> argument has no effect on mappings and is excluded from this
/// struct declaration. |noremap| is included, since it behaves like a map
/// argument when used in a mapping.
///
/// @see mapblock_T
typedef struct map_arguments {
  bool buffer;
  bool expr;
  bool noremap;
  bool nowait;
  bool script;
  bool silent;
  bool unique;

  /// The {lhs} of the mapping.
  ///
  /// vim limits this to MAXMAPLEN characters, allowing us to use a static
  /// buffer. Setting lhs_len to a value larger than MAXMAPLEN can signal
  /// that {lhs} was too long and truncated.
  char_u lhs[MAXMAPLEN + 1];
  size_t lhs_len;

  char_u *rhs;  /// The {rhs} of the mapping.
  size_t rhs_len;
  LuaRef rhs_lua;  /// lua function as rhs
  bool rhs_is_noop;  /// True when the {orig_rhs} is <nop>.

  char_u *orig_rhs;  /// The original text of the {rhs}.
  size_t orig_rhs_len;
  char *desc;  /// map description
} MapArguments;

/// Possible values for map_arguments.buffer
/// Values > 2 are for user-defined scopes
#define MAP_IS_GLOBAL 0
#define MAP_IS_BUFFER 1
#define MAP_IS_WINDOW 2

#define MAP_ARGUMENTS_INIT {  \
  .buffer =       false,      \
  .expr =         false,      \
  .noremap =      false,      \
  .nowait =       false,      \
  .script =       false,      \
  .silent =       false,      \
  .unique =       false,      \
  .lhs =          { 0 },      \
  .lhs_len =      0,          \
  .rhs =          NULL,       \
  .rhs_len =      0,          \
  .rhs_lua =      LUA_NOREF,  \
  .rhs_is_noop =  false,      \
  .orig_rhs =     NULL,       \
  .orig_rhs_len = 0,          \
  .desc =         NULL,       \
}


/// Argument type for the function do_map()
typedef enum {
  DoMapCmd_map = 0,     // |:map|
  DoMapCmd_unmap = 1,   // |:unmap|
  DoMapCmd_noremap = 2, // |:noremap|
} DoMapCmd;

/// Possible result codes from do_map()
typedef enum {
  DoMapResult_unknown_error        = -1,
  DoMapResult_success              =  0,
  DoMapResult_invalid_arguments    =  1,
  DoMapResult_no_match             =  2,
  DoMapResult_entry_is_not_unique  =  5,
} DoMapResult;

typedef enum {
  DoMapCompare_unequal,     // The strings do not match.
  DoMapCompare_exact_match, // The strings match exactly.
  DoMapCompare_lhs_matches_initial_chars_of_rhs,  // e.g. foo vs foobar
  DoMapCompare_rhs_matches_initial_chars_of_lhs,  // e.g. foobar vs foo
} DoMapCompare;


#define KEYLEN_PART_KEY -1  // keylen value for incomplete key-code
#define KEYLEN_PART_MAP -2  // keylen value for incomplete mapping


#ifdef INCLUDE_GENERATED_DECLARATIONS
# include "maphash.h.generated.h"
#endif
#endif  // NVIM_MAPHASH_H
