#include <stdarg.h>
#include <unistd.h>

#include <errno.h>

#include <glib/gfileutils.h>
#include <glib/gstrfuncs.h>
#include <glib/gtestutils.h>

#include <tinu/reporting.h>
#include <tinu/log.h>

/* Options */
static const gchar *g_opt_program_name = NULL;
static const gchar *g_opt_program_extra_opts = "";

static const gchar *g_opt_filename = NULL;

/* Globals */
static gchar g_prg_prefix[4096];

static void
_prg_report_set(const gchar *fmt, ...)
{
  va_list vl;

  va_start(vl, fmt);
  vsprintf(g_prg_prefix, fmt, vl);
  va_end(vl);
}

static void
_prg_report_print(FILE *file, const gchar *fmt, ...)
{
  va_list vl;

  va_start(vl, fmt);
  fprintf(file, "%s.", g_prg_prefix);
  vfprintf(file, fmt, vl);
  fprintf(file, "\n");
  va_end(vl);
}

static FILE *
_prg_report_passfile(gchar **name)
{
  gchar tpl[] = "/tmp/tinu-pass-XXXXXX";
  gint fd = g_mkstemp(tpl);
  FILE *res = fdopen(fd, "w");

  g_assert(res);

  *name = g_strdup(tpl);
  return res;
}

static gboolean
test_report_program_check(StatisticsVerbosity verbosity, gboolean enable_colour)
{
  if (!g_opt_program_name)
    {
      log_error("Missing program name from program report", NULL);
      return FALSE;
    }
  return TRUE;
}

static void
_test_report_put_file(FILE *file, TestStatistics *stat)
{
  gint i, j;
  double tics = (double)sysconf(_SC_CLK_TCK);

  StatSuiteInfo *suite;
  StatTestInfo *test;

  _prg_report_set("summary");
  _prg_report_print(file, "passed=%d", stat->m_passed);
  _prg_report_print(file, "failed=%d", stat->m_failed);
  _prg_report_print(file, "segfault=%d", stat->m_sigsegv);

  for (i = 0; i < stat->m_suite_info_list->len; i++)
    {
      suite = &g_array_index(stat->m_suite_info_list, StatSuiteInfo, i);

      _prg_report_set("suite.%s", suite->m_suite->m_name);
      _prg_report_print(file, "result=%d", suite->m_result ? 1 : 0);
      _prg_report_print(file, "asserts.passed=%d", suite->m_assertions_passed);
      _prg_report_print(file, "asserts.total=%d", suite->m_assertions);
      _prg_report_print(file, "time=%lf", (suite->m_end - suite->m_start) / tics);

      for (j = 0; j < suite->m_test_info_list->len; j++)
        {
          test = &g_array_index(suite->m_test_info_list, StatTestInfo, j);

          _prg_report_set("suite.%s.test.%s", suite->m_suite->m_name, test->m_test->m_name);
          _prg_report_print(file, "result=%d", (test->m_result == TEST_SEGFAULT ? -1 : (test->m_result == TEST_FAILED ? 0 : 1)));
          _prg_report_print(file, "asserts.passed=%d", test->m_assertions_passed);
          _prg_report_print(file, "asserts.total=%d", test->m_assertions);
          _prg_report_print(file, "time=%lf", (test->m_end - test->m_start) / tics);
        }
    }

  _prg_report_set("message");
  for (i = LOG_CRIT; i <= LOG_DEBUG; i++)
    _prg_report_print(file, "%s=%d", msg_format_priority(i), stat->m_messages[i]);
}

static void
test_report_program(TestStatistics *stat, StatisticsVerbosity verbosity, gboolean enable_colour)
{
  gchar *fname;
  FILE *file = _prg_report_passfile(&fname);

  gchar temp[4096];

  _test_report_put_file(file, stat);

  fclose(file);

  snprintf(temp, sizeof(temp), "%s %s %s", g_opt_program_name, fname, g_opt_program_extra_opts);
  log_debug("Running statistics",
            msg_tag_str("program", g_opt_program_name),
            msg_tag_str("command", temp),
            NULL);

  system(temp);
  unlink(fname);
  g_free(fname);
}

const GOptionEntry g_report_program_module_options[] = {
  { "program", 0, 0, G_OPTION_ARG_STRING, (gpointer)&g_opt_program_name,
    "The program to pass the statistics to", NULL },
  { "program-opts", 0, 0, G_OPTION_ARG_STRING, (gpointer)&g_opt_program_extra_opts,
    "Extra arguments to pass the program", NULL },
  { NULL }
};

const ReportModule g_report_progam_module = {
  .m_name = "program",
  .m_options = g_report_program_module_options,

  .m_check = &test_report_program_check,
  .m_handle = &test_report_program
};

static gboolean
test_report_file_check(StatisticsVerbosity verbosity, gboolean enable_colour)
{
  gint _errno;

  if (!g_opt_filename)
    {
      log_error("Missing destination name from file report", NULL);
      return FALSE;
    }
  if (access(g_opt_filename, W_OK) == -1)
    {
      _errno = errno;
      log_error("Cannot access file",
                msg_tag_str("name", g_opt_filename),
                msg_tag_int("code", _errno),
                msg_tag_str("reason", strerror(_errno)), NULL);
      return FALSE;
    }
  return TRUE;
}

static void
test_report_file(TestStatistics *stat, StatisticsVerbosity verbosity, gboolean enable_colour)
{
  FILE *file = fopen(g_opt_filename, "w");

  if (!file)
    {
      log_crit("Could not open file for file report",
               msg_tag_str("", g_opt_filename), NULL);
      g_assert(0);
    }

  _test_report_put_file(file, stat);
  fclose(file);
}

const GOptionEntry g_report_file_module_options[] = {
  { "file-destination", 0, 0, G_OPTION_ARG_STRING, (gpointer)&g_opt_filename,
    "Destination file", NULL },
  { NULL }
};

const ReportModule g_report_file_module = {
  .m_name = "file",
  .m_options = g_report_file_module_options,

  .m_check = &test_report_file_check,
  .m_handle = &test_report_file
};

