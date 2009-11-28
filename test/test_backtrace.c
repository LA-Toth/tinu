#include <stdio.h>

#include <tinu/backtrace.h>
#include <tinu/log.h>

void
other_function()
{
  Backtrace *trace = backtrace_create(1);

  backtrace_dump_file(trace, stdout, 4);
  backtrace_dump_log(trace, "Dump", LOG_INFO);
  backtrace_unreference(trace);
}

void
internim_function()
{
  other_function();
}

int
main()
{
  log_register_message_handler(msg_stderr_handler, LOG_DEBUG, LOGMSG_PROPAGATE);
  internim_function();
  return 0;
}
