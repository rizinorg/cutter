#ifndef CUTTERTREEWIDGET_H
#define CUTTERTREEWIDGET_H

#include "core/CutterCommon.h"

#include <QStatusBar>
#include <QVBoxLayout>

class MainWindow;

class CUTTER_EXPORT CutterTreeWidget : public QObject
{

    Q_OBJECT

#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
#    define Q_DISABLE_COPY(CutterTreeWidget)                                                       \
        CutterTreeWidget(const CutterTreeWidget &w) = delete;                                      \
        CutterTreeWidget &operator=(const CutterTreeWidget &w) = delete;

#    define Q_DISABLE_MOVE(CutterTreeWidget)                                                       \
        CutterTreeWidget(CutterTreeWidget &&w) = delete;                                           \
        CutterTreeWidget &operator=(CutterTreeWidget &&w) = delete;

#    define Q_DISABLE_COPY_MOVE(CutterTreeWidget)                                                  \
        Q_DISABLE_COPY(CutterTreeWidget)                                                           \
        Q_DISABLE_MOVE(CutterTreeWidget)
#endif

    Q_DISABLE_COPY_MOVE(CutterTreeWidget)

public:
    explicit CutterTreeWidget(QObject *parent = nullptr);
    ~CutterTreeWidget() override;
    void addStatusBar(QVBoxLayout *pos);
    void showItemsNumber(int count);
    void showStatusBar(bool show);

private:
    QStatusBar *bar;
};
#endif // CUTTERTREEWIDGET_H
