#ifndef CUTTER_TEST_CALLBACK_TASK_H
#define CUTTER_TEST_CALLBACK_TASK_H

#include "common/AsyncTask.h"

#include <functional>
#include <utility>

namespace CutterTest {

class CallbackTask : public AsyncTask
{
public:
    explicit CallbackTask(std::function<void()> callback) : callback(std::move(callback)) {}

protected:
    void runTask() override { callback(); }

private:
    std::function<void()> callback;
};

} // namespace CutterTest

#endif // CUTTER_TEST_CALLBACK_TASK_H
