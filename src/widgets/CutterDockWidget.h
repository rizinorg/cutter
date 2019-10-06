#ifndef CUTTERWIDGET_H
#define CUTTERWIDGET_H

#include <QDockWidget>

#include "common/RefreshDeferrer.h"

class MainWindow;

class CutterDockWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit CutterDockWidget(MainWindow *parent, QAction *action = nullptr);
    ~CutterDockWidget() override;
    bool eventFilter(QObject *object, QEvent *event) override;
    bool isVisibleToUser()      { return isVisibleToUserCurrent; }

    /**
     * @brief Set whether the Widget should be deleted after it is closed.
     * This is especially important for extra widgets.
     */
    void setTransient(bool v)   { isTransient = v; }

    /**
     * @brief Convenience method for creating and registering a RefreshDeferrer without any parameters
     * @param refreshNowFunc lambda taking no parameters, called when a refresh should occur
     */
    template<typename Func>
    RefreshDeferrer *createRefreshDeferrer(Func refreshNowFunc)
    {
        auto *deferrer = new RefreshDeferrer(nullptr, this);
        deferrer->registerFor(this);
        connect(deferrer, &RefreshDeferrer::refreshNow, this, [refreshNowFunc](const RefreshDeferrerParamsResult) {
            refreshNowFunc();
        });
        return deferrer;
    }

    /**
     * @brief Convenience method for creating and registering a RefreshDeferrer with a replacing Accumulator
     * @param replaceIfNull passed to the ReplacingRefreshDeferrerAccumulator
     * @param refreshNowFunc lambda taking a single parameter of type ParamResult, called when a refresh should occur
     */
    template<class ParamResult, typename Func>
    RefreshDeferrer *createReplacingRefreshDeferrer(bool replaceIfNull, Func refreshNowFunc)
    {
        auto *deferrer = new RefreshDeferrer(new ReplacingRefreshDeferrerAccumulator<ParamResult>(replaceIfNull), this);
        deferrer->registerFor(this);
        connect(deferrer, &RefreshDeferrer::refreshNow, this, [refreshNowFunc](const RefreshDeferrerParamsResult paramsResult) {
            auto *result = static_cast<const ParamResult *>(paramsResult);
            refreshNowFunc(result);
        });
        return deferrer;
    }

signals:
    void becameVisibleToUser();
    void closed();

public slots:
    void toggleDockWidget(bool show);

protected:
    virtual QWidget* widgetToFocusOnRaise();

    void closeEvent(QCloseEvent *event) override;
    QAction *getBoundAction() const;
    QString getDockNumber();

    MainWindow *mainWindow;

private:
    QAction *action;

    bool isTransient = false;

    bool isVisibleToUserCurrent = false;
    void updateIsVisibleToUser();
};

#endif // CUTTERWIDGET_H
