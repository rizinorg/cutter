
#ifndef REFRESHDEFERRER_H
#define REFRESHDEFERRER_H

#include <QObject>

class CutterDockWidget;
class RefreshDeferrer;

using RefreshDeferrerParams = void *;
using RefreshDeferrerParamsResult = void *;

/**
 * @brief Abstract class for accumulating params in RefreshDeferrer
 */
class RefreshDeferrerAccumulator
{
    friend class RefreshDeferrer;

public:
    virtual ~RefreshDeferrerAccumulator() = default;

protected:
    /**
     * @brief Add a new param to the accumulator
     */
    virtual void accumulate(RefreshDeferrerParams params) =0;

    /**
     * @brief Ignore the incoming params. Useful for freeing if necessary.
     */
    virtual void ignoreParams(RefreshDeferrerParams params) =0;

    /**
     * @brief Clear the current accumulator
     */
    virtual void clear() =0;

    /**
     * @brief Return the final result of the accumulation
     */
    virtual RefreshDeferrerParamsResult result() =0;
};


/**
 * @brief Accumulator which simply replaces the current value by an incoming new one
 * @tparam T The type of the param to store
 *
 * This accumulator takes the ownership of all params passed to it and deletes them automatically if not needed anymore!
 */
template<class T>
class ReplacingRefreshDeferrerAccumulator: public RefreshDeferrerAccumulator
{
private:
    T *value = nullptr;
    bool replaceIfNull;

public:
    /**
     * \param Determines whether, if nullptr is passed, the current value should be replaced or kept.
     */
    explicit ReplacingRefreshDeferrerAccumulator(bool replaceIfNull = true)
        : replaceIfNull(replaceIfNull) {}

    ~ReplacingRefreshDeferrerAccumulator() override
    {
        delete value;
    }

protected:
    void accumulate(RefreshDeferrerParams params) override
    {
        if (!replaceIfNull && !params) {
            return;
        }
        delete value;
        value = static_cast<T *>(params);
    }

    void ignoreParams(RefreshDeferrerParams params) override
    {
        delete static_cast<T *>(params);
    }

    void clear() override
    {
        delete value;
        value = nullptr;
    }

    virtual RefreshDeferrerParamsResult result() override
    {
        return value;
    }
};

/**
 * @brief Helper class for deferred refreshing in Widgets
 *
 * This class can handle the logic necessary to defer the refreshing of widgets when they are not visible.
 * It contains an optional RefreshDeferrerAccumulator, which can be used to accumulate incoming events while
 * refreshing is deferred.
 *
 * Example (don't write it like this in practice, use the convenience methods in CutterDockWidget):
 * ```
 * // in the constructor of a widget
 * this->refreshDeferrer = new RefreshDeferrer(new ReplacingRefreshDeferrerAccumulator(false), this);
 * this->refreshDeferrer->registerFor(this);
 * connect(this->refreshDeferrer, &RefreshDeferrer::refreshNow, this, [this](MyParam *param) {
 *      // We attempted a refresh some time before, but it got deferred.
 *      // Now the RefreshDeferrer tells us to do the refresh and gives us the accumulated param.
 *      this->doRefresh(*param);
 * }
 *
 * // ...
 *
 * void MyWidget::doRefresh(MyParam param)
 * {
 *      if (!this->refreshDeferrer->attemptRefresh(new MyParam(param))) {
 *          // We shouldn't refresh right now.
 *          // The RefreshDeferrer takes over the param we passed it in attemptRefresh()
 *          // and gives it to the ReplacingRefreshDeferrerAccumulator.
 *          return;
 *      }
 *      // do the actual refresh depending on param
 * }
 * ```
 *
 */
class RefreshDeferrer : public QObject
{
    Q_OBJECT

private:
    CutterDockWidget *dockWidget = nullptr;
    RefreshDeferrerAccumulator *acc;
    bool dirty = false;

public:
    /**
     * @param acc The accumulator (can be nullptr). The RefreshDeferrer takes the ownership!
     */
    explicit RefreshDeferrer(RefreshDeferrerAccumulator *acc, QObject *parent = nullptr);
    ~RefreshDeferrer() override;

    bool attemptRefresh(RefreshDeferrerParams params);
    void registerFor(CutterDockWidget *dockWidget);

signals:
    void refreshNow(const RefreshDeferrerParamsResult paramsResult);
};

#endif //REFRESHDEFERRER_H
