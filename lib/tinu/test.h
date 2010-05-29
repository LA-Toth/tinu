/* TINU - Unittesting framework
*
* Copyright (c) 2009, Viktor Hercinger <hercinger.viktor@gmail.com>
* All rights reserved.
* 
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the original author (Viktor Hercinger) nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
* Author(s): Viktor Hercinger <hercinger.viktor@gmail.com>
*/

/** @file test.h
 * @brief Test cases and their internal representations
 *
 * This file contains the API for the individual test cases.
 */
#ifndef _TINU_TEST_H
#define _TINU_TEST_H

#include <stdlib.h>
#include <string.h>

#include <glib/gtypes.h>
#include <glib/garray.h>

#include <tinu/log.h>

__BEGIN_DECLS

typedef struct _TestCase TestCase;
typedef struct _TestSuite TestSuite;
typedef struct _TestContext TestContext;

/** @brief Setup function
 * The result is the _context_ of the test, meaning that it will be passed to
 * the test and later cleaned up. The rationale behind this is that the same
 * dataset can be prepared and cleaned up for different tests.
 */
typedef gpointer (*TestSetup)(void);
/** @brief Cleanup function
 * This function is used to clean up the previously prepared context. You should
 * free the allocated memory if neccessary.
 */
typedef void (*TestCleanup)(gpointer);
/** @brief Test function
 * TestFunction is the main body of the test case. Generaly speaking it should be
 * independent of the preparation/cleanup (i.e. it should work independently of the 
 * actual initialization as much as possible).
 */
typedef void (*TestFunction)(gpointer);

/**
 * Signifies the result of a given test case and suite.
 */
typedef enum
{
  /** The test case passed */
  TEST_PASSED = 0,
  /** The test case failed */
  TEST_FAILED,
  /** The test case resulted in a segmentation fault */
  TEST_SEGFAULT,
  /** The test case failed because some internal error */
  TEST_INTERNAL,
} TestCaseResult;

/** @brief Individual test case
 *
 * Representation of an individual test case.
 *
 * @note Do not use directly. It may change at any time.
 */
struct _TestCase
{
  /** The suite the case belongs to */
  TestSuite      *m_suite;

  /** Displayed name of the case */
  const gchar    *m_name;

  /** Setup function of the test. Called first. */
  TestSetup       m_setup;
  /** Cleanup function of the test. Called last. */
  TestCleanup     m_cleanup;
  /** Test function. This should be the "body" of the test */
  TestFunction    m_test;

  /** After the test ran, this will continue the result. */
  TestCaseResult  m_result;
};

/** @brief Test suite
 *
 * Suites are the groups in which test cases are handled. They are supposed
 * to help the grouping of a test. They can be executed separately with
 * --suite <suite name> from the command line.
 *
 * @note Do not use directly. It may change at any time.
 */
struct _TestSuite
{
  /** Displayed name of the test suite */
  const gchar    *m_name;

  /** Contains the individual test cases */
  GPtrArray      *m_tests;

  /** If all test resulted in TEST_PASSED, the suite passes. If
   * even one test case fails in any way, the suite fails too.
   */
  gboolean        m_passed;
};

/** @brief Test statistics
 *
 * Contains statistical information about the test cases and
 * emitted messages.
 *
 * @note Do not use directly. It may change at any time.
 */
typedef struct _TestStatistics
{
  /** Number of emitted messages by priority */
  guint32       m_messages[LOG_DEBUG + 1];
  /** Number of segmentation faults */
  guint32       m_sigsegv;

  /** Number of tests passed */
  guint32       m_passed;
  /** Number of tests failed */
  guint32       m_failed;
} TestStatistics;

/** @brief Test context
 *
 * This is the context of a test: it contains all the test suites (which contain the test
 * cases), neccessary information about running the tests, etc.
 *
 * @note Do not use directly. It may change at any time.
 */
struct _TestContext
{
  /** List of suites */
  GPtrArray      *m_suites;

  /** Statistics about the test */
  TestStatistics  m_statistics;

  /** Enables/disables signal handling */
  gboolean        m_sighandle;
  /** Enables/disables built-in leak detection */
  gboolean        m_leakwatch;

  /** Directory to put the cores in (or NULL if no cores
   * should be stored
   */
  const gchar    *m_core_dir;
};

/** @brief Initialize test context
 * @param self The initialized context
 *
 * Initializes the context. Parameters (like m_sighandle or m_sigsegv)
 * should be set manually.
 */
void test_context_init(TestContext *self);
/** @brief Clean up test context
 * @param self Test context
 */
void test_context_destroy(TestContext *self);

/** @brief Add test to context
 * @param self Test context
 * @param suite_name Suite test belongs to
 * @param test_name Name of the current test
 * @param setup Setup function (NULL if none)
 * @param cleanup Cleanup function (NULL if none)
 * @param func Test function (NULL if none)
 *
 * This function is used to fill a test context with test cases. Suites are
 * handled implicitly, so there is no need to add them separately.
 */
void test_add(TestContext *self,
              const gchar *suite_name,
              const gchar *test_name,
              TestSetup setup,
              TestCleanup cleanup,
              TestFunction func);

/** @brief Run all tests
 * @param self Test context
 * @return Wheter all tests succeeded.
 *
 * Runs all tests and collects the statistics
 */
gboolean tinu_test_all_run(TestContext *self);
/** @brief Run an individual suite
 * @param self Test context
 * @param suite_name Suite to run
 * @return Wheter the test suite succeeded.
 *
 * This runs only a distinct test suite. Suites are accessed by name.
 */
gboolean tinu_test_suite_run(TestContext *self, const gchar *suite_name);

/** @brief Evaluate an assertion
 * @param condition The condition to be asserted.
 * @param assert_type A string that will be set in the message as the assert type
 * (e.g. string compare, assert true, assert false, etc.)
 * @param condstr The condition converted to a string.
 * @param file The file where the assertion comes from
 * @param func The function where the assertion comes from
 * @param line The line where the assertion comes from 
 * @param tag0 First tag to add to any message or NULL if none are appended
 * @param ... Additional tags, NULL-terminated
 *
 * Before this function came into effect all assertions were macros,
 * but this proved to be inflexible when hooking came into the picture.
 *
 * @note Do not use directly! Use the macros. This way the API does not
 * change. This function can change.
 */
void tinu_test_assert(gboolean condition, const gchar *assert_type, const gchar *condstr,
  const gchar *file, const gchar *func, gint line, MessageTag *tag0, ...);

/** @brief `TRUE' (or positive) Assertion
 * @param cond Assertion condition
 *
 * This macro checks the condition given and fails (that is, emits
 * a SIGABRT) if the condition is not met.
 */
#define TINU_ASSERT_TRUE(cond) \
  tinu_test_assert((cond), "positive", #cond, __FILE__, __PRETTY_FUNCTION__, __LINE__, NULL);

/** @brief `FALSE' (or negative) Assertion
 * @param cond Assertion condition
 *
 * This macro checks the condition given and fails (that is, emits
 * a SIGABRT) if the condition is met.
 */
#define TINU_ASSERT_FALSE(cond) \
  tinu_test_assert(!(cond), "negative", #cond, __FILE__, __PRETTY_FUNCTION__, __LINE__, NULL);

/** @brief Check if the two strings are equal
 * @param str1 First string
 * @param str2 Second string
 *
 * This macro checks wheter two strings are equal or not
 */
#define TINU_ASSERT_STREQ(str1, str2) \
  tinu_test_assert(strcmp((str1), (str2)) == 0, "string equality", "str1 == str2", \
    __FILE__, __PRETTY_FUNCTION__, __LINE__, \
    msg_tag_str("str1", str1), msg_tag_str("str2", str2), NULL);

__END_DECLS

#endif
