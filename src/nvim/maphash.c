#include "maphash.h"
#include "nvim/lua/executor.h"

// Each mapping is put in one of the MAX_MAPHASH hash lists,
// to speed up finding it.
mapblock_T *(maphash[MAX_MAPHASH]);

/*
 * Delete one entry from the abbrlist or maphash[].
 * mpp is a pointer to the m_next field of the PREVIOUS entry,
 * or, in the case where *mpp is the head element of the list,
 * mpp == (mb_table + hash), for some mb_table and some hash.
 * Return the next element (NULL if the list is empty).
 */
mapblock_T **mapblock_free(mapblock_T **mpp)
{
  mapblock_T *mp;

  mp = *mpp;
  xfree(mp->m_keys);
  NLUA_CLEAR_REF(mp->m_luaref);
  XFREE_CLEAR(mp->m_str);
  XFREE_CLEAR(mp->m_orig_str);
  XFREE_CLEAR(mp->m_desc);
  *mpp = mp->m_next;
  xfree(mp);
  return mpp;
}

