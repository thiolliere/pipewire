/* Pinos
 * Copyright (C) 2016 Wim Taymans <wim.taymans@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/syscall.h>

#include <pinos/client/log.h>
#include <pinos/client/mem.h>

/*
 * No glibc wrappers exist for memfd_create(2), so provide our own.
 *
 * Also define memfd fcntl sealing macros. While they are already
 * defined in the kernel header file <linux/fcntl.h>, that file as
 * a whole conflicts with the original glibc header <fnctl.h>.
 */

static inline int memfd_create(const char *name, unsigned int flags) {
    return syscall(SYS_memfd_create, name, flags);
}

/* memfd_create(2) flags */

#ifndef MFD_CLOEXEC
#define MFD_CLOEXEC       0x0001U
#endif

#ifndef MFD_ALLOW_SEALING
#define MFD_ALLOW_SEALING 0x0002U
#endif

/* fcntl() seals-related flags */

#ifndef F_LINUX_SPECIFIC_BASE
#define F_LINUX_SPECIFIC_BASE 1024
#endif

#ifndef F_ADD_SEALS
#define F_ADD_SEALS (F_LINUX_SPECIFIC_BASE + 9)
#define F_GET_SEALS (F_LINUX_SPECIFIC_BASE + 10)

#define F_SEAL_SEAL     0x0001  /* prevent further seals from being set */
#define F_SEAL_SHRINK   0x0002  /* prevent file from shrinking */
#define F_SEAL_GROW     0x0004  /* prevent file from growing */
#define F_SEAL_WRITE    0x0008  /* prevent writes */
#endif


#undef USE_MEMFD

SpaResult
pinos_memblock_alloc (PinosMemblockFlags  flags,
                      size_t              size,
                      PinosMemblock      *mem)
{
  if (mem == NULL || size == 0)
    return SPA_RESULT_INVALID_ARGUMENTS;

  mem->flags = flags;
  mem->size = size;

  if (flags & PINOS_MEMBLOCK_FLAG_WITH_FD) {
#ifdef USE_MEMFD
    mem->fd = memfd_create ("pinos-memfd", MFD_CLOEXEC | MFD_ALLOW_SEALING);
    if (mem->fd == -1) {
      pinos_log_error ("Failed to create memfd: %s\n", strerror (errno));
      return SPA_RESULT_ERRNO;
    }
#else
    char filename[] = "/dev/shm/spa-tmpfile.XXXXXX";
    mem->fd = mkostemp (filename, O_CLOEXEC);
    if (mem->fd == -1) {
      pinos_log_error ("Failed to create temporary file: %s\n", strerror (errno));
      return SPA_RESULT_ERRNO;
    }
    unlink (filename);
#endif

    if (ftruncate (mem->fd, size) < 0) {
      pinos_log_warn ("Failed to truncate temporary file: %s", strerror (errno));
      close (mem->fd);
      return SPA_RESULT_ERRNO;
    }
#ifdef USE_MEMFD
    if (flags & PINOS_MEMBLOCK_FLAG_SEAL) {
      unsigned int seals = F_SEAL_GROW | F_SEAL_SHRINK | F_SEAL_SEAL;
      if (fcntl (mem->fd, F_ADD_SEALS, seals) == -1) {
        pinos_log_warn ("Failed to add seals: %s", strerror (errno));
      }
    }
#endif
    if (flags & PINOS_MEMBLOCK_FLAG_MAP_READWRITE) {
      int prot = 0;

      if (flags & PINOS_MEMBLOCK_FLAG_MAP_READ)
        prot |= PROT_READ;
      if (flags & PINOS_MEMBLOCK_FLAG_MAP_WRITE)
        prot |= PROT_WRITE;

      mem->ptr = mmap (NULL, size, prot, MAP_SHARED, mem->fd, 0);
      if (mem->ptr == MAP_FAILED)
        return SPA_RESULT_NO_MEMORY;
    } else {
      mem->ptr = NULL;
    }
  } else {
    mem->ptr = malloc (size);
    if (mem->ptr == NULL)
      return SPA_RESULT_NO_MEMORY;
    mem->fd = -1;
  }
  return SPA_RESULT_OK;
}

void
pinos_memblock_free  (PinosMemblock *mem)
{
  if (mem == NULL)
    return;

  if (mem->flags & PINOS_MEMBLOCK_FLAG_WITH_FD) {
    if (mem->ptr)
      munmap (mem->ptr, mem->size);
    if (mem->fd != -1)
      close (mem->fd);
  } else {
    free (mem->ptr);
  }
  mem->ptr = NULL;
  mem->fd = -1;
}
