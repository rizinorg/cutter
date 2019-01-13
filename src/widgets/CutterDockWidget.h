#ifndef CUTTERWIDGET_H
#define CUTTERWIDGET_H

#include <QDockWidget>

#include "common/RefreshDeferrer.h"

class MainWindow;

class CutterDockWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit CutterDockWidget(MainWindow *main, QAction *action = nullptr);
    ~CutterDockWidget() override;
    bool eventFilter(QObject *object, QEvent *event) override;
    bool isVisibleToUser()      { return isVisibleToUserCurrent; }

public slots:
    void toggleDockWidget(bool show);

signals:
    void becameVisibleToUser();

private:
    QAction *action;

    bool isVisibleToUserCurrent;
    void updateIsVisibleToUser();

protected:
    void closeEvent(QCloseEvent *event) override;

    template<class ParamResult, typename Func>
    RefreshDeferrer *createReplacingRefreshDeferrer(Func refreshNowFunc)
    {
        auto *deferrer = new RefreshDeferrer(new ReplacingRefreshDeferrerAccumulator<ParamResult>(), this);
        deferrer->registerFor(this);
        connect(deferrer, &RefreshDeferrer::refreshNow, this, [refreshNowFunc](const RefreshDeferrerParamsResult paramsResult) {
            printf("got refresh now!\n");
            auto *offset = static_cast<const ParamResult *>(paramsResult);
            refreshNowFunc(offset);
        });
        return deferrer;
    }
};

#endif // CUTTERWIDGET_H
