#ifndef COMMANDTASK_H
#define COMMANDTASK_H

#include "common/AsyncTask.h"

/**
 * @brief Background task for running a command in rizin
 */
class CUTTER_EXPORT CommandTask : public AsyncTask
{
    Q_OBJECT

public:
    enum ColorMode : ut8 {
        DISABLED = COLOR_MODE_DISABLED,
        MODE_16 = COLOR_MODE_16,
        MODE_256 = COLOR_MODE_256,
        MODE_16M = COLOR_MODE_16M
    };

    CommandTask(const QString &cmd, ColorMode colorMode = ColorMode::DISABLED);

    QString getTitle() const override { return tr("Running Command"); }

signals:
    void finished(const QString &result);

protected:
    void runTask() override;

private:
    QString cmd;
    ColorMode colorMode;
};

#endif // COMMANDTASK_H
