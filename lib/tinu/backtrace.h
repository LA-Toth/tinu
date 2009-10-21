#ifndef _TINU_BACKTRACE_H
#define _TINU_BACKTRACE_H

#include <stdint.h>
#include <stdio.h>

#include <tinu/log.h>

#ifdef ENABLE_THREADS
#include <glib/gthread.h>
#endif
#include <glib/garray.h>

typedef struct _BacktraceEntry
{
  gpointer    m_ptr;
  gsize       m_offset;

  gchar      *m_function;
  gchar      *m_file;
} BacktraceEntry;

#define BACKTRACE_ENTRY_INVALID          ((BacktraceEntry *)-1)
typedef void (*DumpCallback)(const BacktraceEntry *entry, gpointer ud);

#define MAX_DEPTH       4096

typedef struct _Backtrace Backtrace;

Backtrace *backtrace_create(guint32 skip);
Backtrace *backtrace_create_depth(guint32 depth, guint32 skip);
Backtrace *backtrace_reference(Backtrace *self);
void backtrace_unreference(Backtrace *self);

#ifdef ENABLE_THREADS
#   define backtrace_lock(backtrace)      g_mutex_lock((backtrace)->m_lock);
#   define backtrace_unlock(backtrace)    g_mutex_unlock((backtrace)->m_lock);
#else
#   define backtrace_lock(backtrace)
#   define backtrace_unlock(backtrace)
#endif

guint32 backtrace_depth(const Backtrace *self);
const gchar *backtrace_line(const Backtrace *self, guint32 line);

void backtrace_dump_log(const Backtrace *self, const gchar *msg_prefix, gint priority);
void backtrace_dump_file(const Backtrace *self, FILE *output, guint8 indent);
void backtrace_dump(const Backtrace *self, DumpCallback callback, gpointer user_data);

gboolean backtrace_entry_parse(BacktraceEntry *self, const gchar *line);
void backtrace_entry_destroy(BacktraceEntry *self);

MessageTag *msg_tag_trace(const gchar *tag, const Backtrace *trace);
MessageTag *msg_tag_trace_current(const gchar *tag, int skip);

gboolean backtrace_serialize(Backtrace *self, GByteArray *dest);
Backtrace *backtrace_deserialize(const GByteArray *src);

#endif
