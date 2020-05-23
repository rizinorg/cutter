#ifndef CUTTERWIDGET_H
#define CUTTERWIDGET_H

#include "CutterCommon.h"
#include "common/RefreshDeferrer.h"

#include <QDockWidget>

class MainWindow;

class CutterDockWidget : public QDockWidget
{
    Q_OBJECT

public:
    CUTTER_DEPRECATED("Action will be ignored. Use CutterDockWidget(MainWindow*) instead.")
    CutterDockWidget(MainWindow *parent, QAction *action);

    explicit CutterDockWidget(MainWindow *parent);
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
    /**
     * @brief Serialize dock properties for saving as part of layout.
     *
     * Override this function for saving dock specific view properties. Use
     * in situations where it makes sense to have different properties for
     * multiple instances of widget. Don't use for options that are more suitable
     * as global settings and should be applied equally to all widgets or all
     * widgets of this kind.
     *
     * Keep synchrononized with deserializeViewProperties. When modifying add
     * project upgrade step in SettingsUpgrade.cpp if necessary.
     *
     * @return Dictionary of current dock properties.
     * @see CutterDockWidget#deserializeViewProperties
     */
    virtual QVariantMap serializeViewProprties();
    /**
     * @brief Deserialization half of serialize view properties.
     *
     * When a property is not specified in property map dock should reset it
     * to default value instead of leaving it umodified. Empty map should reset
     * all properties controlled by serializeViewProprties/deserializeViewProperties
     * mechanism.
     *
     * @param properties to modify for current widget
     * @see CutterDockWidget#serializeViewProprties
     */
    virtual void deserializeViewProperties(const QVariantMap &properties);
signals:
    void becameVisibleToUser();
    void closed();

public slots:
    void toggleDockWidget(bool show);

protected:
    virtual QWidget* widgetToFocusOnRaise();

    void closeEvent(QCloseEvent *event) override;
    QString getDockNumber();

    MainWindow *mainWindow;

private:
    bool isTransient = false;

    bool isVisibleToUserCurrent = false;
    void updateIsVisibleToUser();
};

#endif // CUTTERWIDGET_H
