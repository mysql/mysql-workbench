/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA 
 */

#include "grt/grt_dispatcher.h"
#include "grt/grt_manager.h"
#include "wb_test_helpers.h"

#include "casmine.h"

namespace {

$ModuleEnvironment() {};

using namespace grt;
using namespace bec;

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

$TestData {
  GRTDispatcher::Ref dispatcher;
};

$describe("GRT request dispatcher") {

  $beforeAll([this]() {
    // No need to initialize Python for this test. No need for a module path either.
    grt::GRT::get(); // make sure grt is initialized before Dispatcher is created
    data->dispatcher = GRTDispatcher::create_dispatcher(false, true);
    data->dispatcher->start();
  });

  $afterAll([this]() {
    data->dispatcher->shutdown();
    data->dispatcher.reset();
  });


  $it("Testing callbacks", [this](){
    grt::ValueRef result;
    bool finish_called = false;

    bec::GRTTask::Ref task = GRTTask::create_task("test", data->dispatcher, std::bind(normal_test_function));
    task->signal_finished()->connect(std::bind(&finished, std::placeholders::_1, &finish_called));

    result = data->dispatcher->add_task_and_wait(task);

    $expect(result.is_valid()).toBeTrue();
    $expect(result.type()).toEqual(grt::IntegerType);
    $expect(*grt::IntegerRef::cast_from(result)).toBe(123);

    $expect(finish_called).toBeTrue();

    finish_called = false;
    task = GRTTask::create_task("test", data->dispatcher, std::bind(normal_test_function));
    task->signal_finished()->connect(std::bind(&finished_with_wait, std::placeholders::_1, &finish_called));

    result = data->dispatcher->add_task_and_wait(task);

    $expect(finish_called).toBeTrue();
  });

}

}
