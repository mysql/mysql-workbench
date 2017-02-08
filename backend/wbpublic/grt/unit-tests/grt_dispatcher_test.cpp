/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "grt/grt_dispatcher.h"
#include "grt/grt_manager.h"
#include "wb_helpers.h"

using namespace grt;
using namespace bec;

BEGIN_TEST_DATA_CLASS(grt_dispatcher_test)
public:
GRTDispatcher::Ref _dispatcher;

TEST_DATA_CONSTRUCTOR(grt_dispatcher_test) {
  // No need to initialize Python for this test. No need for a module path either.
  grt::GRT::get(); // make sure grt is initialized before Dispatcher is created
  _dispatcher = GRTDispatcher::create_dispatcher(false, true);
  _dispatcher->start();
}

END_TEST_DATA_CLASS;

TEST_MODULE(grt_dispatcher_test, "grt request dispatcher");

static void finished(grt::ValueRef result, bool *flag) {
  *flag = true;
}

static void finished_with_wait(grt::ValueRef result, bool *flag) {
  g_usleep(2000000);
  *flag = true;
}

static grt::ValueRef normal_test_function() {
  return grt::IntegerRef(123);
}

TEST_FUNCTION(1) {
  // test callbacks
  grt::ValueRef result;
  bool finish_called = false;

  bec::GRTTask::Ref task = GRTTask::create_task("test", _dispatcher, std::bind(normal_test_function));
  task->signal_finished()->connect(std::bind(&finished, std::placeholders::_1, &finish_called));

  result = _dispatcher->add_task_and_wait(task);

  ensure("result", result.is_valid() && result.type() == grt::IntegerType);
  ensure_equals("result value", *grt::IntegerRef::cast_from(result), 123);

  ensure("finish callback called", finish_called);

  finish_called = false;
  task = GRTTask::create_task("test", _dispatcher, std::bind(normal_test_function));
  task->signal_finished()->connect(std::bind(&finished_with_wait, std::placeholders::_1, &finish_called));

  result = _dispatcher->add_task_and_wait(task);

  ensure("finish callback called with wait", finish_called);
}

TEST_FUNCTION(5) {
  // test msg queue
}

/*
TEST_FUNCTION(6)
{
  // test task calling another task

}
*/

TEST_FUNCTION(99) {
  // we need to shutdown it here instead of d-tor because it will crash otherwise
  _dispatcher->shutdown();
  _dispatcher.reset();
}

END_TESTS
