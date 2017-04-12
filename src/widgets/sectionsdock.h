#ifndef SECTIONSDOCK_H
#define SECTIONSDOCK_H

#include <QDockWidget>
#include "widgets/sectionswidget.h"

class MainWindow;

namespace Ui
{
    class SectionsDock;
}

class SectionsDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit SectionsDock(MainWindow *main, QWidget *parent = 0);
    ~SectionsDock();

    SectionsWidget  *sectionsWidget;

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:

    void showSectionsContextMenu(const QPoint &pt);

    void on_actionVertical_triggered();

    void on_actionHorizontal_triggered();

private:
    Ui::SectionsDock *ui;

    MainWindow      *main;
};

#endif // SECTIONSDOCK_H
