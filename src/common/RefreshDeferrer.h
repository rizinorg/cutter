
#ifndef REFRESHDEFERRER_H
#define REFRESHDEFERRER_H

#include <QObject>

class CutterDockWidget;
class RefreshDeferrer;

using RefreshDeferrerParams = void *;
using RefreshDeferrerParamsResult = void *;

class RefreshDeferrerAccumulator
{
    friend class RefreshDeferrer;

public:
    virtual ~RefreshDeferrerAccumulator() = default;

protected:
    virtual void accumulate(RefreshDeferrerParams params) =0;
    virtual void ignoreParams(RefreshDeferrerParams params) =0;
    virtual void clear() =0;
    virtual RefreshDeferrerParamsResult result() =0;
};

template<class T>
class ReplacingRefreshDeferrerAccumulator: public RefreshDeferrerAccumulator
{
private:
    T *value = nullptr;

public:
    ~ReplacingRefreshDeferrerAccumulator() override
    {
        delete value;
    }

protected:
    void accumulate(RefreshDeferrerParams params) override
    {
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

class RefreshDeferrer : public QObject
{
    Q_OBJECT

private:
    CutterDockWidget *dockWidget = nullptr;
    RefreshDeferrerAccumulator *acc;
    bool dirty = false;

public:
    RefreshDeferrer(RefreshDeferrerAccumulator *acc, QObject *parent = nullptr);
    virtual ~RefreshDeferrer();

    bool attemptRefresh(RefreshDeferrerParams params);
    void registerFor(CutterDockWidget *dockWidget);

signals:
    void refreshNow(const RefreshDeferrerParamsResult paramsResult);
};

#endif //REFRESHDEFERRER_H
